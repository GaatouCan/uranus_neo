#include "Connection.h"
#include "Network.h"
#include "Server.h"
#include "Utils.h"
#include "login/LoginAuth.h"
#include "gateway/Gateway.h"
#include "base/PackageCodec.h"

#include <asio/experimental/awaitable_operators.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>


using namespace asio::experimental::awaitable_operators;
using namespace std::literals::chrono_literals;


UConnection::UConnection(unique_ptr<IPackageCodec_Interface> &&codec)
    : mCodec(std::move(codec)),
      mChannel(mCodec->GetSocket().get_executor(), 1024),
      mWatchdog(mCodec->GetSocket().get_executor()),
      mExpiration(std::chrono::seconds(30)) {

    GetSocket().set_option(asio::ip::tcp::no_delay(true));
    GetSocket().set_option(asio::ip::tcp::socket::keep_alive(true));

    mKey = fmt::format("{}-{}",mCodec->GetSocket().remote_endpoint().address().to_string(), utils::UnixTime());
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
}

asio::ip::address UConnection::RemoteAddress() const {
    if (IsSocketOpen()) {
        return GetSocket().remote_endpoint().address();
    }
    return {};
}

const std::string &UConnection::GetKey() const {
    return mKey;
}

void UConnection::SendPackage(const FPackageHandle &pkg) {
    if (pkg == nullptr)
        return;

    if (const bool ret = mChannel.try_send_via_dispatch(std::error_code{}, pkg); !ret) {
        co_spawn(GetSocket().get_executor(), [self = shared_from_this(), pkg]() -> awaitable<void> {
            co_await self->mChannel.async_send(std::error_code{}, pkg);
        }, detached);
    }
}


awaitable<void> UConnection::WritePackage() {
    try {
        while (IsSocketOpen()) {
            const auto [ec, pkg] = co_await mChannel.async_receive();
            if (ec || pkg == nullptr)
                co_return;

            if (const auto ret = co_await mCodec->Encode(pkg.Get()); !ret) {
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
    try {
        while (IsSocketOpen()) {
            const auto pkg = BuildPackage();
            if (pkg == nullptr)
                co_return;

            if (const auto ret = co_await mCodec->Decode(pkg.Get()); !ret) {
                Disconnect();
                break;
            }

            const auto now = std::chrono::steady_clock::now();

            // Run The Login Branch
            // if (mPlayerID < 0) {
            //     // Do Not Try Login Too Frequently
            //     if (now - mReceiveTime > std::chrono::seconds(3)) {
            //         --mPlayerID;
            //
            //         // Try Login Failed 3 Times Then Disconnect This
            //         if (mPlayerID < -3) {
            //             SPDLOG_WARN("{:<20} - Connection[{}] Try Login Too Many Times", __FUNCTION__, RemoteAddress().to_string());
            //             Disconnect();
            //             break;
            //         }
            //
            //         // Handle Login Logic
            //         if (auto *login = GetServer()->GetModule<ULoginAuth>(); login != nullptr) {
            //             login->OnLoginRequest(mKey, pkg);
            //         }
            //     }
            //
            //     mReceiveTime = now;
            //     continue;
            // }

            // Update Receive Time Point For Watchdog
            mReceiveTime = now;

            // const auto *gateway = GetServer()->GetModule<UGateway>();
            // const auto *auth = GetServer()->GetModule<ULoginAuth>();
            //
            // if (gateway != nullptr && auth != nullptr) {
            //     switch (pkg->GetPackageID()) {
            //         case HEARTBEAT_PACKAGE_ID: {
            //             gateway->OnHeartBeat(mPlayerID, pkg);
            //         }
            //             break;
            //         case LOGIN_REQUEST_PACKAGE_ID: break;
            //         case PLATFORM_PACKAGE_ID: {
            //             auth->OnPlatformInfo(mPlayerID, pkg);
            //         }
            //             break;
            //         default:
            //             gateway->OnClientPackage(mPlayerID, pkg);
            //     }
            // }
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
        } while (mReceiveTime + mExpiration > now);

        if (IsSocketOpen()) {
            SPDLOG_WARN("{:<20} - Watchdog Timeout", __FUNCTION__);
            Disconnect();
        }
    } catch (const std::exception &e) {
        SPDLOG_ERROR("{:<20} - {}", __FUNCTION__, e.what());
    }
}
