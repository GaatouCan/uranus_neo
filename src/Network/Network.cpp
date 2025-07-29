#include "Network.h"
#include "Connection.h"
#include "Server.h"
#include "Base/Package.h"
#include "Config/Config.h"
#include "Login/LoginAuth.h"
#include "Utils.h"

#include <string>
#include <spdlog/spdlog.h>


// static const std::string key = "1d0fa7879b018b71a00109e7e29eb0c5037617e9eddc3d46e95294fc1fa3ad50";

UNetwork::UNetwork()
    : mAcceptor(mIOContext),
      mPackagePool(nullptr) {
}

UNetwork::~UNetwork() {
    Stop();

    if (mThread.joinable()) {
        mThread.join();
    }
}

void UNetwork::Initial() {
    if (mState != EModuleState::CREATED)
        return;

    mPackagePool = make_shared<TRecycler<FPackage>>();
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

awaitable<void> UNetwork::WaitForClient(uint16_t port) {
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

                const auto conn = make_shared<UConnection>(std::move(socket));
                conn->SetUpModule(this);

                if (const auto id = conn->GetConnectionID(); id > 0) {
                    std::unique_lock lock(mMutex);
                    mConnectionMap[id] = conn;
                } else {
                    SPDLOG_WARN("{:<20} - Failed To Get Connection ID From {}",
                        __FUNCTION__, conn->RemoteAddress().to_string());
                    conn->Disconnect();
                    continue;
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

std::shared_ptr<FPackage> UNetwork::BuildPackage() const {
    if (mState != EModuleState::RUNNING)
        return nullptr;

    return std::dynamic_pointer_cast<FPackage>(mPackagePool->Acquire());
}

shared_ptr<UConnection> UNetwork::FindConnection(const int64_t cid) const {
    if (mState != EModuleState::RUNNING)
        return nullptr;

    std::shared_lock lock(mMutex);
    const auto it = mConnectionMap.find(cid);
    return it != mConnectionMap.end() ? it->second : nullptr;
}

void UNetwork::RemoveConnection(const int64_t cid, const int64_t pid) {
    if (mState != EModuleState::RUNNING)
        return;

    {
        std::unique_lock lock(mMutex);
        mConnectionMap.erase(cid);
    }

    if (pid > 0) {
        // if (auto *gateway = GetServer()->GetModule<UGateway>()) {
        //     gateway->OnPlayerLogout(pid);
        // }
    }
}

void UNetwork::SendToClient(const int64_t cid, const shared_ptr<FPackage> &pkg) const {
    if (mState != EModuleState::RUNNING)
        return;

    std::shared_lock lock(mMutex);
    if (const auto iter = mConnectionMap.find(cid); iter != mConnectionMap.end()) {
        if (const auto conn = iter->second) {
            conn->SendPackage(pkg);
        }
    }
}

void UNetwork::OnLoginSuccess(const int64_t cid, const int64_t pid, const shared_ptr<FPackage> &pkg) const {
    if (mState != EModuleState::RUNNING)
        return;

    const shared_ptr<UConnection> conn = FindConnection(cid);
    if (conn == nullptr) return;

    conn->SetPlayerID(pid);
    conn->SendPackage(pkg);
}

void UNetwork::OnLoginFailure(const int64_t cid, const shared_ptr<FPackage> &pkg) const {
    if (mState != EModuleState::RUNNING)
        return;

    const shared_ptr<UConnection> conn = FindConnection(cid);
    if (conn == nullptr) return;

    conn->SendPackage(pkg);
    conn->Disconnect();
}
