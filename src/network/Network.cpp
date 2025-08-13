#include "Network.h"
#include "base/CodecFactory.h"
#include "base/Package.h"
#include "Connection.h"
#include "Server.h"
#include "config/Config.h"
#include "login/LoginAuth.h"
#include "Utils.h"

#include <spdlog/spdlog.h>


UNetwork::UNetwork()
    : mAcceptor(mIOContext),
      mNextIndex(0),
      mCodecFactory(nullptr) {
}

UNetwork::~UNetwork() {
    Stop();

    for (auto &[th, ctx] : mWorkerList) {
        if (th.joinable()) {
            th.join();
        }
    }
}

void UNetwork::Initial() {
    if (mState != EModuleState::CREATED)
        return;

    assert(mCodecFactory != nullptr);

    mWorkerList = std::vector<FNetworkNode>(4);
    for (auto &[th, ctx] : mWorkerList) {
        th = std::thread([&ctx]() {
            asio::signal_set signals(ctx, SIGINT, SIGTERM);
            signals.async_wait([&ctx](auto, auto) {
                ctx.stop();
            });
            ctx.run();
        });
    }

    mState = EModuleState::INITIALIZED;
}

void UNetwork::Start() {
    if (mState != EModuleState::INITIALIZED)
        return;

    uint16_t port = 8080;
    if (const auto *config = GetServer()->GetModule<UConfig>(); config != nullptr) {
        port = config->GetServerConfig()["server"]["port"].as<uint16_t>();
    }

    asio::signal_set signals(mIOContext, SIGINT, SIGTERM);
    signals.async_wait([this](auto, auto) {
        GetServer()->Shutdown();
    });

    co_spawn(mIOContext, WaitForClient(port), detached);
    mState = EModuleState::RUNNING;

    mIOContext.run();
}

void UNetwork::Stop() {
    if (mState == EModuleState::STOPPED)
        return;

    mState = EModuleState::STOPPED;

    if (!mIOContext.stopped()) {
        mIOContext.stop();
    }

    mConnectionMap.clear();
}

io_context &UNetwork::GetIOContext() {
    return mIOContext;
}

shared_ptr<IRecyclerBase> UNetwork::CreatePackagePool() const {
    if (mCodecFactory)
        return mCodecFactory->CreatePackagePool();
    return nullptr;
}

awaitable<void> UNetwork::WaitForClient(uint16_t port) {
    try {
        mAcceptor.open(asio::ip::tcp::v4());
        mAcceptor.bind({asio::ip::tcp::v4(), port});
        mAcceptor.listen(port);

        SPDLOG_INFO("Waiting For Client To Connect - Server Port: {}", port);

        while (mState == EModuleState::RUNNING) {
            auto [ec, socket] = co_await mAcceptor.async_accept(GetNextIOContext());

            if (ec) {
                SPDLOG_ERROR("{:<20} - {}", __FUNCTION__, ec.message());
                continue;
            }

            if (socket.is_open()) {
                if (auto *login = GetServer()->GetModule<ULoginAuth>(); login != nullptr) {
                    if (!login->VerifyAddress(socket.remote_endpoint())) {
                        SPDLOG_WARN("Reject Client From {}", socket.remote_endpoint().address().to_string());
                        socket.close();
                        continue;
                    }
                }

                const auto id = mIDAllocator.AllocateTS();
                {
                    std::shared_lock lock(mConnectionMutex);
                    if (mConnectionMap.contains(id)) {
                        SPDLOG_ERROR("{:<20} - Connection ID Repeated.", __FUNCTION__);
                        socket.close();
                        continue;
                    }
                }

                const auto conn = make_shared<UConnection>(mCodecFactory->CreatePackageCodec(std::move(socket)));
                conn->SetConnectionID(id);
                conn->SetUpModule(this);

                {
                    std::unique_lock lock(mConnectionMutex);
                    mConnectionMap[id] = conn;
                }

                SPDLOG_INFO("{:<20} - New Connection From {} - fd[{}]",
                    __FUNCTION__, conn->RemoteAddress().to_string(), conn->GetConnectionID());

                conn->ConnectToClient();
            }
        }
    } catch (const std::exception &e) {
        SPDLOG_ERROR("{} - {}", __FUNCTION__, e.what());
    }
}

shared_ptr<UConnection> UNetwork::FindConnection(const int64_t cid) const {
    if (mState != EModuleState::RUNNING)
        return nullptr;

    std::shared_lock lock(mConnectionMutex);
    const auto it = mConnectionMap.find(cid);
    return it != mConnectionMap.end() ? it->second : nullptr;
}

void UNetwork::RemoveConnection(const int64_t cid, const int64_t pid) {
    if (mState != EModuleState::RUNNING)
        return;

    {
        std::unique_lock lock(mConnectionMutex);
        mConnectionMap.erase(cid);
    }

    if (pid > 0) {
        // if (auto *gateway = GetServer()->GetModule<UGateway>()) {
        //     gateway->OnPlayerLogout(pid);
        // }
    }
}

void UNetwork::SendToClient(const int64_t cid, const shared_ptr<IPackage_Interface> &pkg) const {
    if (mState != EModuleState::RUNNING)
        return;

    std::shared_lock lock(mConnectionMutex);
    if (const auto iter = mConnectionMap.find(cid); iter != mConnectionMap.end()) {
        if (const auto conn = iter->second) {
            conn->SendPackage(pkg);
        }
    }
}

void UNetwork::OnLoginSuccess(const int64_t cid, const int64_t pid, const shared_ptr<IPackage_Interface> &pkg) const {
    if (mState != EModuleState::RUNNING)
        return;

    const shared_ptr<UConnection> conn = FindConnection(cid);
    if (conn == nullptr) return;

    conn->SetPlayerID(pid);
    conn->SendPackage(pkg);
}

void UNetwork::OnLoginFailure(const int64_t cid, const shared_ptr<IPackage_Interface> &pkg) const {
    if (mState != EModuleState::RUNNING)
        return;

    const shared_ptr<UConnection> conn = FindConnection(cid);
    if (conn == nullptr) return;

    conn->SendPackage(pkg);
    conn->Disconnect();
}

void UNetwork::RecycleID(const int64_t cid) {
    if (mState != EModuleState::RUNNING)
        return;

    mIDAllocator.RecycleTS(cid);
}

io_context &UNetwork::GetNextIOContext() {
    if (mWorkerList.empty()) {
        return mIOContext;
    }

    auto &[th, ctx] = mWorkerList[mNextIndex++];
    mNextIndex = mNextIndex % mWorkerList.size();
    return ctx;
}
