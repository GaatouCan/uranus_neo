#include "Server.h"
#include "base/PackageCodec.h"
#include "base/Recycler.h"
#include "Agent.h"
#include "PlayerBase.h"
#include "login/LoginAuth.h"

#include <spdlog/spdlog.h>
#include <format>

UServer::UServer()
    : mAcceptor(mIOContext),
      mState(EServerState::CREATED) {
}

UServer::~UServer() {
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

    mState = EServerState::INITIALIZED;
}

void UServer::Start() {
    if (mState != EServerState::INITIALIZED)
        throw std::logic_error(std::format("{} - Server Not In INITIALIZED State", __FUNCTION__));

    for (const auto &pModule: mModuleOrder) {
        pModule->Start();
    }

    mIOContextPool.Start(4);
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

    for (auto iter = mModuleOrder.rbegin(); iter != mModuleOrder.rend(); ++iter) {
        (*iter)->Stop();
    }
}

shared_ptr<UAgent> UServer::FindAgent(const int64_t pid) const {
    if (mState != EServerState::RUNNING)
        return nullptr;

    std::shared_lock lock(mAgentMutex);
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

void UServer::RemoveAgent(const int64_t pid) {
    if (mState != EServerState::RUNNING)
        return;

    shared_ptr<UAgent> agent;
    {
        std::unique_lock lock(mAgentMutex);
        if (const auto iter = mPlayerMap.find(pid); iter != mPlayerMap.end()) {
            agent = iter->second;
            mPlayerMap.erase(iter);
        }
    }

    if (!agent)
        return;

    // TODO: Cached Player Instance
}

void UServer::OnPlayerLogin(const std::string &key, const int64_t pid) {
    if (mState != EServerState::RUNNING)
        return;

    unique_ptr<IPlayerBase> player;

    shared_ptr<UAgent> agent;
    shared_ptr<UAgent> existed;

    {
        std::unique_lock lock(mAgentMutex);
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

    if (existed) {
        existed->Disconnect();
    }

    if (!agent)
        return;

    if (player) {
        player->Save();
        player->OnRepeat();
    } else {
        // TODO: Create Player Instance
    }

    agent->SetUpPlayer(std::move(player));
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

awaitable<void> UServer::WaitForClient(uint16_t port) {
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
                agent->SetUpAgent(this);

                bool bSuccess = true;

                if (const auto key = agent->GetKey(); !key.empty()) {
                    std::unique_lock lock(mAgentMutex);
                    if (mAgentMap.contains(key)) {
                        SPDLOG_WARN("{:<20} - Connection[{}] Has Already Exist.", __FUNCTION__, key);
                        bSuccess = false;
                    } else {
                        bSuccess = true;
                        mAgentMap.insert_or_assign(key, agent);
                    }
                } else {
                    SPDLOG_WARN("{:<20} - Failed To Get Connection ID From {}",
                        __FUNCTION__, agent->RemoteAddress().to_string());

                    bSuccess = false;
                    continue;
                }

                if (!bSuccess) {
                    agent->Disconnect();
                    continue;
                }

                SPDLOG_INFO("{:<20} - New Connection From {} - key[{}]",
                    __FUNCTION__, agent->RemoteAddress().to_string(), agent->GetKey());

                agent->ConnectToClient();
            }
        }
    } catch (const std::exception &e) {
        SPDLOG_ERROR("{} - {}", __FUNCTION__, e.what());
    }
}
