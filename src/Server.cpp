#include "Server.h"
#include "Agent.h"
#include "Context.h"
#include "base/PackageCodec.h"
#include "base/Recycler.h"
#include "base/AgentWorker.h"
#include "config/Config.h"
#include "login/LoginAuth.h"

#include <spdlog/spdlog.h>
#include <ranges>
#include <format>


UServer::UServer()
    : mAcceptor(mIOContext),
      mCacheTimer(mIOContext),
      mState(EServerState::CREATED) {
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

    mPlayerFactory->Initial();
    mServiceFactory->LoadService();

    for (const auto &val : cfg["services"]["core"]) {
        const auto path = val["path"].as<std::string>();
        const auto library = mServiceFactory->FindService(path);
        if (!library.IsValid()) {
            SPDLOG_CRITICAL("Failed To Find Service[{}]", path);
            continue;
        }

        const FServiceHandle sid = mAllocator.Allocate();

        if (mServiceMap.contains(sid)) [[unlikely]]
            throw std::logic_error(fmt::format("Allocate Same Service ID[{}]", sid.id));

        const auto context = make_shared<UContext>(mWorkerPool.GetIOContext());
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

    mIOContextPool.Start(4);
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

    mCacheTimer.cancel();

    if (mIOContext.stopped()) {
        mIOContext.stop();
    }

    mAgentMap.clear();
    mPlayerMap.clear();
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

asio::io_context &UServer::GetWorkerContext() {
    return mWorkerPool.GetIOContext();
}

shared_ptr<UAgent> UServer::FindPlayer(const int64_t pid) const {
    if (mState != EServerState::RUNNING)
        return nullptr;

    std::shared_lock lock(mPlayerMutex);
    const auto iter = mPlayerMap.find(pid);
    return iter == mPlayerMap.end() ? nullptr : iter->second;
}

shared_ptr<UAgent> UServer::FindAgent(const std::string &key) const {
    if (mState != EServerState::RUNNING)
        return nullptr;

    std::shared_lock lock(mAgentMutex);
    const auto iter = mAgentMap.find(key);
    return iter == mAgentMap.end() ? nullptr : iter->second;
}

std::vector<shared_ptr<UAgent>> UServer::GetPlayerList(const std::vector<int64_t> &list) const {
    if (mState != EServerState::RUNNING)
        return {};

    std::vector<shared_ptr<UAgent>> result;
    std::shared_lock lock(mPlayerMutex);
    for (const auto &pid : list) {
        if (const auto iter = mPlayerMap.find(pid); iter != mPlayerMap.end()) {
            result.push_back(iter->second);
        }
    }

    return result;
}

void UServer::RemovePlayer(const int64_t pid) {
    if (mState != EServerState::RUNNING)
        return;

    std::unique_lock lock(mPlayerMutex);
    mPlayerMap.erase(pid);
}

void UServer::RecyclePlayer(FPlayerHandle &&player) {
    if (mState != EServerState::RUNNING)
        return;

    if (player == nullptr)
        return;

    const auto pid = player->GetPlayerID();
    if (pid <= 0)
        return;

    this->RemovePlayer(pid);

    std::unique_lock lock(mCacheMutex);
    mCachedMap.insert_or_assign(pid, FCachedNode{
        std::move(player),
        std::chrono::steady_clock::now()
    });
}

void UServer::RemoveAgent(const std::string &key) {
    if (mState != EServerState::RUNNING)
        return;

    std::unique_lock lock(mAgentMutex);
    mAgentMap.erase(key);
}

void UServer::OnPlayerLogin(const std::string &key, const int64_t pid) {
    if (mState != EServerState::RUNNING)
        return;

    FPlayerHandle player;

    shared_ptr<UAgent> agent;
    shared_ptr<UAgent> existed;

    {
        std::scoped_lock lock(mAgentMutex, mPlayerMutex);
        if (const auto iter = mPlayerMap.find(pid); iter != mPlayerMap.end()) {
            if (iter->second->GetKey() == key)
                return;

            player = std::move(iter->second->ExtractPlayer());
            existed = iter->second;
            mPlayerMap.erase(iter);
        }

        if (const auto iter = mAgentMap.find(key); iter != mAgentMap.end()) {
            agent = iter->second;
            mAgentMap.erase(iter);
            mPlayerMap.insert_or_assign(pid, agent);
        }
    }

    // Query The Cached Map
    {
        std::unique_lock lock(mCacheMutex);
        if (player == nullptr) {
            if (const auto iter = mCachedMap.find(pid); iter != mCachedMap.end()) {
                player = std::move(iter->second.player);
                mCachedMap.erase(iter);
            }
        }

        mCachedMap.erase(pid);
    }

    if (!agent)
        return;

    if (existed) {
        existed->OnRepeated(agent->RemoteAddress().to_string());
    }

    if (player) {
        player->Save();
        player->OnReset();
    } else {
        player = mPlayerFactory->CreatePlayer();
    }

    agent->SetUpPlayer(std::move(player));
}

shared_ptr<UContext> UServer::FindService(const FServiceHandle &sid) const {
    if (mState != EServerState::RUNNING)
        return nullptr;

    std::shared_lock lock(mServiceMutex);
    const auto iter = mServiceMap.find(sid);
    return iter == mServiceMap.end() ? nullptr : iter->second;
}

shared_ptr<UContext> UServer::FindService(const std::string &name) const {
    if (mState != EServerState::RUNNING)
        return nullptr;

    FServiceHandle handle;

    {
        std::shared_lock lock(mServiceNameMutex);
        const auto iter = mServiceNameMap.find(name);
        if (iter == mServiceNameMap.end())
            return nullptr;

        handle = iter->second;
    }

    return FindService(handle);
}

void UServer::BootService(const std::string &path, const IDataAsset_Interface *pData) {
    if (mState != EServerState::RUNNING || mServiceFactory == nullptr)
        return;

    const auto library = mServiceFactory->FindService(path);
    if (!library.IsValid()) {
        SPDLOG_ERROR("{} - Fail To Find Service Library[{}]", __FUNCTION__, path);
        return;
    }

    const FServiceHandle sid = mAllocator.AllocateTS();

    // Check If The Service Handle Is Valid
    {
        std::shared_lock lock(mServiceMutex);
        if (mServiceMap.contains(sid)) {
            SPDLOG_CRITICAL("{} - Service Handle Repeated, [{}]", __FUNCTION__, sid.id);
            return;
        }
    }

    const auto context = make_shared<UContext>(mWorkerPool.GetIOContext());

    context->SetUpServer(this);
    context->SetUpServiceID(sid);
    context->SetUpLibrary(library);

    if (!context->Initial(pData)) {
        SPDLOG_ERROR("{} - Service[{}] Initialize Fail", __FUNCTION__, sid.id);
        context->Stop();
        mAllocator.RecycleTS(sid);
        return;
    }

    const std::string name = context->GetServiceName();

    // Check If The Service Name Is Defined
    if (name.empty() || name == "UNKNOWN") {
        SPDLOG_ERROR("{} - Service[{}] Name Undefined", __FUNCTION__, sid.id);
        context->Stop();
        mAllocator.RecycleTS(sid);
        return;
    }

    // Check If The Service Name Is Unique
    bool bRepeat = false;
    {
        std::shared_lock lock(mServiceNameMutex);
        if (mServiceNameMap.contains(name)) {
            bRepeat = true;
        }
    }

    if (bRepeat || !context->BootService()) {
        SPDLOG_ERROR("{} - Service[{}] Name Repeated Or Fail To Boot", __FUNCTION__, name);
        context->Stop();
        mAllocator.RecycleTS(sid.id);
        return;
    }

    SPDLOG_INFO("{} - Boot Service[{}] Successfully", __FUNCTION__, name);

    std::scoped_lock lock(mServiceMutex, mServiceNameMutex);
    mServiceMap.insert_or_assign(sid, context);
    mServiceNameMap.insert_or_assign(name, sid);
}

void UServer::ShutdownService(const FServiceHandle &handle) {
    if (mState != EServerState::RUNNING)
        return;

    shared_ptr<UContext> context;

    // Erase From Service Map
    {
        std::unique_lock lock(mServiceMutex);
        const auto iter = mServiceMap.find(handle);
        if (iter == mServiceMap.end())
            return;

        context = iter->second;
        mServiceMap.erase(iter);
    }

    if (context == nullptr)
        return;

    const std::string name = context->GetServiceName();

    // Erase From Name Mapping
    {
        std::shared_lock lock(mServiceNameMutex);
        mServiceNameMap.erase(name);
    }

    // Recycle The Service Handle
    mAllocator.RecycleTS(handle);

    SPDLOG_INFO("{} - Stop The Service[{}, {}]", __FUNCTION__, handle.id, name);
    context->Stop();
}

void UServer::ShutdownService(const std::string &name) {
    if (mState != EServerState::RUNNING)
        return;

    FServiceHandle handle;

    // Get The Service Handle And Erase From The Name Mapping
    {
        std::unique_lock lock(mServiceNameMutex);
        const auto iter = mServiceNameMap.find(name);
        if (iter == mServiceNameMap.end())
            return;

        handle = iter->second;
        mServiceNameMap.erase(iter);
    }

    shared_ptr<UContext> context;

    // Erase From The Service Map
    {
        std::unique_lock lock(mServiceMutex);
        const auto iter = mServiceMap.find(handle);
        if (iter == mServiceMap.end())
            return;

        context = iter->second;
        mServiceMap.erase(iter);
    }

    // Recycle The Service Handle
    mAllocator.RecycleTS(handle);

    if (context == nullptr)
        return;

    // If The Target Name Not Equal
    if (context->GetServiceName() != name) {
        SPDLOG_CRITICAL("{} - Service[{}] Name Not Equal[{} - {}]", __FUNCTION__, handle.id, name, context->GetServiceName());
    }

    SPDLOG_INFO("{} - Stop The Service[{}, {}]", __FUNCTION__, handle.id, context->GetServiceName());
    context->Stop();
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

FPlayerHandle UServer::CreatePlayer() const {
    if (mPlayerFactory == nullptr)
        return {};

    return mPlayerFactory->CreatePlayer();
}

unique_ptr<IAgentWorker> UServer::CreateAgentWorker() const {
    if (mPlayerFactory == nullptr)
        return nullptr;

    return mPlayerFactory->CreateAgentWorker();
}

awaitable<void> UServer::WaitForClient(const uint16_t port) {
    try {
        mAcceptor.open(asio::ip::tcp::v4());
        mAcceptor.bind({asio::ip::tcp::v4(), port});
        mAcceptor.listen(port);

        SPDLOG_INFO("Waiting For Client To Connect - Server Port: {}", port);

        while (mState == EServerState::RUNNING) {
            auto [ec, socket] = co_await mAcceptor.async_accept(mIOContextPool.GetIOContext());

            if (ec) {
                SPDLOG_ERROR("{:<20} - {}", __FUNCTION__, ec.message());
                continue;
            }

            if (socket.is_open()) {
                if (auto *login = GetModule<ULoginAuth>(); login != nullptr) {
                    if (!login->VerifyAddress(socket.remote_endpoint())) {
                        SPDLOG_WARN("Reject Client From {}", socket.remote_endpoint().address().to_string());
                        socket.close();
                        continue;
                    }
                }

                const auto agent = make_shared<UAgent>(CreateUniquePackageCodec(std::move(socket)));
                const auto key = agent->GetKey();

                if (key.empty()) {
                    continue;
                }

                {
                    std::unique_lock lock(mAgentMutex);
                    if (mAgentMap.contains(key)) {
                        SPDLOG_WARN("{:<20} - Connection[{}] Has Already Exist.", __FUNCTION__, key);
                        continue;
                    }
                    mAgentMap.insert_or_assign(key, agent);
                }

                SPDLOG_INFO("{:<20} - New Connection From {} - key[{}]",
                    __FUNCTION__, agent->RemoteAddress().to_string(), agent->GetKey());

                agent->SetUpAgent(this);
                agent->ConnectToClient();
            }
        }
    } catch (const std::exception &e) {
        SPDLOG_ERROR("{} - {}", __FUNCTION__, e.what());
    }
}

awaitable<void> UServer::CollectCachedPlayer() {
    try {
        const auto &cfg = GetServerConfig();

        const auto gap = cfg["server"]["cache"]["collect_gap"].as<int>();
        const auto keepSec = cfg["server"]["cache"]["keep_alive"].as<int>();
        const auto maxSize = cfg["server"]["cache"]["max_size"].as<int>();

        const auto duration = std::chrono::seconds(gap);
        auto point = std::chrono::steady_clock::now();

        while (mState != EServerState::TERMINATED) {
            point += duration;
            mCacheTimer.expires_at(point);
            if (auto [ec] = co_await mCacheTimer.async_wait(); ec) {
                SPDLOG_ERROR("{} - {}", __FUNCTION__, ec.message());
                break;
            }

            std::unique_lock lock(mCacheMutex);
            absl::erase_if(mCachedMap, [point, keepSec](const decltype(mCachedMap)::value_type &pair) {
                return point - pair.second.timepoint > std::chrono::seconds(keepSec);
            });

            if (mCachedMap.size() > maxSize) {
                size_t del = mCachedMap.size() - maxSize;
                for (auto iter = mCachedMap.begin(); iter != mCachedMap.end() && del > 0;) {
                    mCachedMap.erase(iter++);
                    --del;
                }
            }
        }

    } catch (std::exception &e) {
        SPDLOG_ERROR("{} - {}", __FUNCTION__, e.what());
    }
}
