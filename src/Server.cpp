#include "Server.h"
#include "base/PackageCodec.h"
#include "config/Config.h"
#include "login/LoginAuth.h"

#include <spdlog/spdlog.h>
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

    if (const auto coreDir = std::filesystem::path(CORE_SERVICE_DIRECTORY); !std::filesystem::exists(coreDir)) {
        try {
            std::filesystem::create_directory(coreDir);
        } catch (const std::exception &e) {
            SPDLOG_ERROR("{} - {}", __FUNCTION__, e.what());
            Shutdown();
            exit(-1);
        }
    }

    if (const auto extendDir = std::filesystem::path(EXTEND_SERVICE_DIRECTORY); !std::filesystem::exists(extendDir)) {
        try {
            std::filesystem::create_directory(extendDir);
        } catch (const std::exception &e) {
            SPDLOG_ERROR("{} - {}", __FUNCTION__, e.what());
            Shutdown();
            exit(-2);
        }
    }

    // Initial All Moduel
    for (const auto &pModule: mModuleOrder) {
        SPDLOG_INFO("Initial Server Module[{}]", pModule->GetModuleName());
        pModule->Initial();
    }

    mState = EServerState::INITIALIZED;
}

void UServer::Start() {
    if (mState != EServerState::INITIALIZED)
        throw std::logic_error(std::format("{} - Server Not In INITIALIZED State", __FUNCTION__));

    const auto &cfg = GetServerConfig();

    // Start All Modules
    for (const auto &pModule: mModuleOrder) {
        SPDLOG_INFO("Start Server Module[{}]", pModule->GetModuleName());
        pModule->Start();
    }

    mState = EServerState::RUNNING;

    asio::signal_set signals(mIOContext, SIGINT, SIGTERM);
    signals.async_wait([this](auto, auto) {
        Shutdown();
    });

    SPDLOG_INFO("Server Is Running");

    // Run The Main IOContext
    mIOContext.run();
}

void UServer::Shutdown() {
    if (mState == EServerState::TERMINATED)
        return;

    mState = EServerState::TERMINATED;
    SPDLOG_INFO("Server Is Shutting Down");

    // Stop The Main IOContext
    if (mIOContext.stopped()) {
        mIOContext.stop();
    }

    // Stop All Modules In Reverse Order
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
