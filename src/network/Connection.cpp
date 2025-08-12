#include "Connection.h"
#include "base/Package.h"
#include "base/PackageCodec.h"
#include "Network.h"
#include "Server.h"
#include "login/LoginAuth.h"
#include "gateway/Gateway.h"
#include "Utils.h"

#include <asio/experimental/awaitable_operators.hpp>
#include <spdlog/spdlog.h>


using namespace asio::experimental::awaitable_operators;
using namespace std::literals::chrono_literals;


UConnection::UConnection(IPackageCodec_Interface *codec)
    : mNetwork(nullptr),
      mCodec(codec),
      mChannel(mCodec->GetSocket().get_executor(), 1024),
      mWatchdog(mCodec->GetSocket().get_executor()),
      mExpiration(std::chrono::seconds(30)),
      mPlayerID(-1) {
    mID = static_cast<int64_t>(GetSocket().native_handle());
    GetSocket().set_option(asio::ip::tcp::no_delay(true));
    GetSocket().set_option(asio::ip::tcp::socket::keep_alive(true));
}

void UConnection::SetUpModule(UNetwork *owner) {
    mNetwork = owner;
}

UConnection::~UConnection() {
    Disconnect();
}

bool UConnection::IsSocketOpen() const {
    return GetSocket().is_open();
}

ATcpSocket &UConnection::GetSocket() const {
    return mCodec->GetSocket();
}

UConnection::APackageChannel &UConnection::GetChannel() {
    return mChannel;
}

void UConnection::SetExpireSecond(const int sec) {
    mExpiration = std::chrono::seconds(sec);
}

void UConnection::SetPlayerID(const int64_t id) {
    if (mPlayerID > 0)
        return;

    mPlayerID = id;
}

void UConnection::ConnectToClient() {
    mReceiveTime = std::chrono::steady_clock::now();

    co_spawn(GetSocket().get_executor(), [self = shared_from_this()]() -> awaitable<void> {
        if (const auto ret = co_await self->mCodec->Initial(); !ret) {
            co_return;
        }

        co_await (
            self->ReadPackage() ||
            self->WritePackage() ||
            self->Watchdog()
        );
    }, detached);
}

void UConnection::Disconnect() {
    if (!IsSocketOpen())
        return;

    mWatchdog.cancel();
    GetSocket().close();

    mNetwork->RemoveConnection(mID, mPlayerID);

    if (mPlayerID > 0) {
        if (GetServer() != nullptr) {
            if (auto *gateway = GetServer()->GetModule<UGateway>()) {
                gateway->OnPlayerLogout(mPlayerID);
            }
        }
        mPlayerID = -1;
    }
}

UNetwork *UConnection::GetNetworkModule() const {
    return mNetwork;
}

UServer *UConnection::GetServer() const {
    if (mNetwork)
        return mNetwork->GetServer();
    return nullptr;
}

shared_ptr<IPackage_Interface> UConnection::BuildPackage() const {
    if (mNetwork)
        return mNetwork->BuildPackage();
    return nullptr;
}

asio::ip::address UConnection::RemoteAddress() const {
    if (IsSocketOpen()) {
        return GetSocket().remote_endpoint().address();
    }
    return {};
}

int64_t UConnection::GetConnectionID() const {
    return mID;
}

int64_t UConnection::GetPlayerID() const {
    return mPlayerID;
}

void UConnection::SendPackage(const shared_ptr<IPackage_Interface> &pkg) {
    if (pkg == nullptr)
        return;

    co_spawn(GetSocket().get_executor(), [self = shared_from_this(), pkg]() -> awaitable<void> {
        co_await self->mChannel.async_send(std::error_code{}, pkg);
    }, detached);
}


awaitable<void> UConnection::WritePackage() {
    if (GetServer() == nullptr) {
        SPDLOG_CRITICAL("{:<20} - Use GetServer() Before Set Up!", __FUNCTION__);
        co_return;
    }

    try {
        while (IsSocketOpen()) {
            const auto [ec, pkg] = co_await mChannel.async_receive();
            if (ec || pkg == nullptr)
                co_return;

            if (const auto ret = co_await mCodec->Encode(pkg); !ret) {
                Disconnect();
                break;
            }

            // Can Do Something Here
        }
    } catch (const std::exception &e) {
        SPDLOG_ERROR("{:<20} - {}", __FUNCTION__, e.what());
        Disconnect();
    }
}

awaitable<void> UConnection::ReadPackage() {
    if (GetServer() == nullptr) {
        SPDLOG_CRITICAL("{:<20} - Use GetServer() Before Set Up!", __FUNCTION__);
        co_return;
    }

    try {
        while (IsSocketOpen()) {
            const auto pkg = BuildPackage();
            if (pkg == nullptr)
                co_return;

            if (const auto ret = co_await mCodec->Decode(pkg); !ret) {
                Disconnect();
                break;
            }

            const auto now = std::chrono::steady_clock::now();

            // Run The Login Branch
            if (mPlayerID < 0) {
                // Do Not Try Login Too Frequently
                if (now - mReceiveTime > std::chrono::seconds(3)) {
                    --mPlayerID;

                    // Try Login Failed 3 Times Then Disconnect This
                    if (mPlayerID < -3) {
                        SPDLOG_WARN("{:<20} - Connection[{}] Try Login Too Many Times", __FUNCTION__, RemoteAddress().to_string());
                        Disconnect();
                        break;
                    }

                    // Handle Login Logic
                    if (auto *login = GetServer()->GetModule<ULoginAuth>(); login != nullptr) {
                        login->OnPlayerLogin(mID, pkg);
                    }
                }

                mReceiveTime = now;
                continue;
            }

            // Update Receive Time Point For Watchdog
            mReceiveTime = now;

            if (const auto *gateway = GetServer()->GetModule<UGateway>(); gateway != nullptr) {
                if (pkg->GetPackageID() == 1001) {
                    gateway->OnHeartBeat(mPlayerID, pkg);
                } else {
                    gateway->OnClientPackage(mPlayerID, pkg);
                }
            }
        }
    } catch (const std::exception &e) {
        SPDLOG_ERROR("{:<20} - {}", __FUNCTION__, e.what());
        Disconnect();
    }
}

awaitable<void> UConnection::Watchdog() {
    if (mExpiration == ASteadyDuration::zero())
        co_return;

    try {
        ASteadyTimePoint now;

        do {
            mWatchdog.expires_at(mReceiveTime + mExpiration);

            if (auto [ec] = co_await mWatchdog.async_wait(); ec) {
                SPDLOG_DEBUG("{:<20} - Timer Canceled.", __FUNCTION__);
                co_return;
            }

            now = std::chrono::steady_clock::now();
        } while ((mReceiveTime + mExpiration) > now);

        if (IsSocketOpen()) {
            SPDLOG_WARN("{:<20} - Watchdog Timeout", __FUNCTION__);
            Disconnect();
        }
    } catch (const std::exception &e) {
        SPDLOG_ERROR("{:<20} - {}", __FUNCTION__, e.what());
    }
}
