#include "Network.h"
#include "base/CodecFactory.h"
#include "Connection.h"
#include "Server.h"
#include "config/Config.h"
#include "login/LoginAuth.h"
#include "monitor/Monitor.h"
#include "Utils.h"

#include <spdlog/spdlog.h>


UNetwork::UNetwork()
    : mAcceptor(mIOContext),
      mCodecFactory(nullptr),
      mPackagePool(nullptr) {
}

UNetwork::~UNetwork() {
    Stop();

    delete mPackagePool;
    delete mCodecFactory;

    if (mThread.joinable()) {
        mThread.join();
    }
}

void UNetwork::Initial() {
    assert(mState == EModuleState::CREATED);
    assert(mCodecFactory != nullptr);

    mPackagePool = mCodecFactory->CreatePackagePool();
    mPackagePool->Initial();

    mThread = std::thread([this] {
       SPDLOG_INFO("Network Work In Thread[{}]", utils::ThreadIDToInt(std::this_thread::get_id()));

       asio::signal_set signals(mIOContext, SIGINT, SIGTERM);
       signals.async_wait([this](auto, auto) {
           Stop();
       });
       mIOContext.run();
   });

    mState = EModuleState::INITIALIZED;
}

void UNetwork::Start() {
    assert(GetServer() != nullptr);

    if (mState != EModuleState::INITIALIZED)
        return;

    uint16_t port = 8080;
    if (const auto *config = GetServer()->GetModule<UConfig>(); config != nullptr) {
        port = config->GetServerConfig()["server"]["port"].as<uint16_t>();
    }

    co_spawn(mIOContext, WaitForClient(port), detached);
    mState = EModuleState::RUNNING;
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

IRecyclerBase *UNetwork::CreatePackagePool() const {
    if (mCodecFactory)
        return mCodecFactory->CreatePackagePool();
    return nullptr;
}

awaitable<void> UNetwork::WaitForClient(uint16_t port) {
    assert(GetServer() != nullptr);

    try {
        mAcceptor.open(asio::ip::tcp::v4());
        mAcceptor.bind({asio::ip::tcp::v4(), port});
        mAcceptor.listen(port);

        SPDLOG_INFO("Waiting For Client To Connect - Server Port: {}", port);

        while (mState == EModuleState::RUNNING) {
            auto [ec, socket] = co_await mAcceptor.async_accept();

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

                const auto conn = make_shared<UConnection>(mCodecFactory->CreatePackageCodec(std::move(socket)));
                conn->SetUpModule(this);

                if (const auto key = conn->GetKey(); !key.empty()) {
                    std::unique_lock lock(mMutex);
                    if (mConnectionMap.contains(key)) {
                        SPDLOG_WARN("{:<20} - Connection[{}] Has Already Exist.", __FUNCTION__, key);
                        conn->Disconnect();
                        continue;
                    }
                    mConnectionMap[key] = conn;
                } else {
                    SPDLOG_WARN("{:<20} - Failed To Get Connection ID From {}",
                        __FUNCTION__, conn->RemoteAddress().to_string());
                    conn->Disconnect();
                    continue;
                }

                SPDLOG_INFO("{:<20} - New Connection From {} - key[{}]",
                    __FUNCTION__, conn->RemoteAddress().to_string(), conn->GetKey());

                conn->ConnectToClient();

                if (auto *monitor = GetServer()->GetModule<UMonitor>(); monitor != nullptr) {
                    monitor->OnAcceptClient(conn);
                }
            }
        }
    } catch (const std::exception &e) {
        SPDLOG_ERROR("{} - {}", __FUNCTION__, e.what());
    }
}

FRecycleHandle<IPackage_Interface> UNetwork::BuildPackage() const {
    assert(mPackagePool != nullptr);

    if (mState != EModuleState::RUNNING)
        return {};

    return mPackagePool->Acquire<IPackage_Interface>();
}

shared_ptr<UConnection> UNetwork::FindConnection(const std::string &key) const {
    if (mState != EModuleState::RUNNING)
        return nullptr;

    std::shared_lock lock(mMutex);
    const auto it = mConnectionMap.find(key);
    return it != mConnectionMap.end() ? it->second : nullptr;
}

void UNetwork::RemoveConnection(const std::string &key, const int64_t pid) {
    if (mState != EModuleState::RUNNING)
        return;

    {
        std::unique_lock lock(mMutex);
        mConnectionMap.erase(key);
    }

    if (pid > 0) {
        // if (auto *gateway = GetServer()->GetModule<UGateway>()) {
        //     gateway->OnPlayerLogout(pid);
        // }
    }
}

void UNetwork::SendToClient(const std::string &key, const FPackageHandle &pkg) const {
    if (mState != EModuleState::RUNNING)
        return;

    std::shared_lock lock(mMutex);
    if (const auto iter = mConnectionMap.find(key); iter != mConnectionMap.end()) {
        if (const auto conn = iter->second) {
            conn->SendPackage(pkg);
        }
    }
}

void UNetwork::OnLoginSuccess(const std::string &key, const int64_t pid, const FPackageHandle &pkg) const {
    if (mState != EModuleState::RUNNING)
        return;

    const shared_ptr<UConnection> conn = FindConnection(key);
    if (conn == nullptr) return;

    conn->SetPlayerID(pid);
    conn->SendPackage(pkg);
}

void UNetwork::OnLoginFailure(const std::string &key, const FPackageHandle &pkg) const {
    if (mState != EModuleState::RUNNING)
        return;

    const shared_ptr<UConnection> conn = FindConnection(key);
    if (conn == nullptr) return;

    conn->SendPackage(pkg);
    conn->Disconnect();
}
