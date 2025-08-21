#include "Server.h"
#include "Agent.h"
#include "Context.h"
#include "base/PackageCodec.h"
#include "base/Recycler.h"
#include "base/AgentHandler.h"
#include "login/LoginAuth.h"

#include <spdlog/spdlog.h>
#include <ranges>
#include <format>


UServer::UServer()
    : mAcceptor(mIOContext),
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
        pModule->Initial();
    }

    // FIXME: Create Core Service From Config
    for (const auto &path : { "gameworld" }) {
        const FServiceHandle sid = mAllocator.Allocate();

        const auto context = make_shared<UContext>(mWorkerPool.GetIOContext());
        context->SetUpServer(this);
        context->SetUpServiceID(sid);

        // TODO: Get Library Handle;

        if (context->Initial(nullptr)) {
            mContextMap.insert_or_assign(sid, context);
        } else {
            SPDLOG_CRITICAL("Failed To Initial Service: {}", path);
            mAllocator.Recycle(sid);
        }
    }

    mIOContextPool.Start(4);
    mWorkerPool.Start(4);

    mState = EServerState::INITIALIZED;
}

void UServer::Start() {
    if (mState != EServerState::INITIALIZED)
        throw std::logic_error(std::format("{} - Server Not In INITIALIZED State", __FUNCTION__));

    for (const auto &pModule: mModuleOrder) {
        pModule->Start();
    }

    for (const auto &context : mContextMap | std::views::values) {
        context->BootService();
    }

    co_spawn(mIOContext, WaitForClient(8080), detached);

    mState = EServerState::RUNNING;

    asio::signal_set signals(mIOContext, SIGINT, SIGTERM);
    signals.async_wait([this](auto, auto) {
        Shutdown();
    });

    mIOContext.run();
}

void UServer::Shutdown() {
    if (mState == EServerState::TERMINATED)
        return;

    mState = EServerState::TERMINATED;

    if (mIOContext.stopped()) {
        mIOContext.stop();
    }

    mAgentMap.clear();
    mPlayerMap.clear();
    mContextMap.clear();

    for (const auto &context : mContextMap | std::views::values) {
        context->Stop();
    }

    for (auto iter = mModuleOrder.rbegin(); iter != mModuleOrder.rend(); ++iter) {
        (*iter)->Stop();
    }
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

void UServer::RemovePlayer(const int64_t pid) {
    if (mState != EServerState::RUNNING)
        return;

    std::unique_lock lock(mPlayerMutex);
    mPlayerMap.erase(pid);
}

void UServer::RecyclePlayer(unique_ptr<IPlayerBase> &&player) {
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

    unique_ptr<IPlayerBase> player;

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

    std::shared_lock lock(mContextMutex);
    const auto iter = mContextMap.find(sid);
    return iter == mContextMap.end() ? nullptr : iter->second;
}

void UServer::BootService(const std::string &path, IDataAsset_Interface *pData) {
    if (mState != EServerState::RUNNING)
        return;

    // TODO: Get Library
}

void UServer::ShutdownService(const FServiceHandle &handle) {
    if (mState != EServerState::RUNNING)
        return;

}

unique_ptr<IPackageCodec_Interface> UServer::CreateUniquePackageCodec(ATcpSocket &&socket) const {
    if (mCodecFactory == nullptr)
        return nullptr;

    return mCodecFactory->CreateUniquePackageCodec(std::move(socket));
}

unique_ptr<IRecyclerBase> UServer::CreateUniquePackagePool() const {
    if (mCodecFactory == nullptr)
        return nullptr;

    return mCodecFactory->CreateUniquePackagePool();
}

unique_ptr<IPlayerBase> UServer::CreatePlayer() const {
    if (mPlayerFactory == nullptr)
        return nullptr;

    return mPlayerFactory->CreatePlayer();
}

unique_ptr<IAgentHandler> UServer::CreateAgentHandler() const {
    if (mPlayerFactory == nullptr)
        return nullptr;

    return mPlayerFactory->CreateAgentHandler();
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
