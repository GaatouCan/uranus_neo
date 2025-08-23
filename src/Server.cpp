#include "Server.h"
#include "gateway/PlayerAgent.h"
#include "service/ServiceAgent.h"
#include "base/PackageCodec.h"
#include "base/Recycler.h"
#include "base/AgentHandler.h"
#include "config/Config.h"
#include "login/LoginAuth.h"

#include <spdlog/spdlog.h>
#include <ranges>
#include <format>


UServer::UServer()
    : mState(EServerState::CREATED) {
}

UServer::~UServer() {
    Shutdown();
}

EServerState UServer::GetState() const {
    return mState;
}

void UServer::Initial() {
    if (mState != EServerState::CREATED)
        throw std::logic_error(std::format("{} - Server Not In CREATED State", __FUNCTION__));

    for (const auto &pModule: mModuleOrder) {
        SPDLOG_INFO("Initial Server Module[{}]", pModule->GetModuleName());
        pModule->Initial();
    }

    const auto &cfg = GetServerConfig();

    mServiceFactory->LoadService();

    for (const auto &val : cfg["services"]["core"]) {
        const auto path = val["path"].as<std::string>();
        const auto library = mServiceFactory->FindService(path);
        if (!library.IsValid()) {
            SPDLOG_CRITICAL("Failed To Find Service[{}]", path);
            continue;
        }

        const auto sid = mAllocator.Allocate();

        if (mServiceMap.contains(sid)) [[unlikely]]
            throw std::logic_error(fmt::format("Allocate Same Service ID[{}]", sid));

        const auto context = make_shared<UServiceAgent>(mWorkerPool.GetIOContext());
        context->SetUpServer(this);
        context->SetUpServiceID(sid);
        context->SetUpLibrary(library);

        if (!context->Initial(nullptr)) {
            SPDLOG_CRITICAL("Failed To Initial Service: {}", path);
            mAllocator.Recycle(sid);
            context->Stop();
            continue;
        }

        const std::string name = context->GetServiceName();
        if (mServiceNameMap.contains(name)) {
            SPDLOG_CRITICAL("Service[{}] Already Exists", name);
            context->Stop();
            continue;
        }

        SPDLOG_INFO("Service[{}] Initialized", name);

        mServiceMap.insert_or_assign(sid, context);
        mServiceNameMap.insert_or_assign(name, sid);
    }

    mWorkerPool.Start(4);

    mState = EServerState::INITIALIZED;
}

void UServer::Start() {
    if (mState != EServerState::INITIALIZED)
        throw std::logic_error(std::format("{} - Server Not In INITIALIZED State", __FUNCTION__));

    const auto &cfg = GetServerConfig();

    for (const auto &pModule: mModuleOrder) {
        SPDLOG_INFO("Start Server Module[{}]", pModule->GetModuleName());
        pModule->Start();
    }

    for (const auto &context : mServiceMap | std::views::values) {
        SPDLOG_INFO("Boot Service[{}]", context->GetServiceName());
        context->BootService();
    }

    const auto port = cfg["server"]["port"].as<uint16_t>();

    co_spawn(mIOContext, WaitForClient(port), detached);
    co_spawn(mIOContext, CollectCachedPlayer(), detached);

    mState = EServerState::RUNNING;

    asio::signal_set signals(mIOContext, SIGINT, SIGTERM);
    signals.async_wait([this](auto, auto) {
        Shutdown();
    });

    SPDLOG_INFO("Server Is Running");
    mIOContext.run();
}

void UServer::Shutdown() {
    if (mState == EServerState::TERMINATED)
        return;

    mState = EServerState::TERMINATED;
    SPDLOG_INFO("Server Is Shutting Down");

    if (mIOContext.stopped()) {
        mIOContext.stop();
    }

    mServiceMap.clear();

    for (const auto &context : mServiceMap | std::views::values) {
        SPDLOG_INFO("Stop Service[{}]", context->GetServiceName());
        context->Stop();
    }

    for (auto iter = mModuleOrder.rbegin(); iter != mModuleOrder.rend(); ++iter) {
        SPDLOG_INFO("Stop Module[{}]", (*iter)->GetModuleName());
        (*iter)->Stop();
    }

    SPDLOG_INFO("Shutdown The Server Successfully");
}

asio::io_context &UServer::GetIOContext() {
    return mIOContext;
}




const YAML::Node &UServer::GetServerConfig() const {
    const auto *config = GetModule<UConfig>();
    if (config == nullptr)
        throw std::runtime_error("Fail To Get Config");

    return config->GetServerConfig();
}

unique_ptr<IPackageCodec_Interface> UServer::CreateUniquePackageCodec(ATcpSocket &&socket) const {
    if (mCodecFactory == nullptr)
        return nullptr;

    return mCodecFactory->CreateUniquePackageCodec(std::move(socket));
}

unique_ptr<IRecyclerBase> UServer::CreateUniquePackagePool(asio::io_context &ctx) const {
    if (mCodecFactory == nullptr)
        return nullptr;

    return mCodecFactory->CreateUniquePackagePool(ctx);
}

ICodecFactory_Interface *UServer::GetCodecFactory() const {
    return mCodecFactory.get();
}

IServiceFactory_Interface *UServer::GetServiceFactory() const {
    return mServiceFactory.get();
}
