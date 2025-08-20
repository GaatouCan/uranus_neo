#include "Agent.h"
#include "Server.h"
#include "PlayerBase.h"
#include "Utils.h"
#include "login/LoginAuth.h"
#include "base/PackageCodec.h"

#include <asio/experimental/awaitable_operators.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>


using namespace asio::experimental::awaitable_operators;
using namespace std::literals::chrono_literals;


UAgent::UAgent(unique_ptr<IPackageCodec_Interface> &&codec)
    : mPackageCodec(std::move(codec)),
      mPackageChannel(mPackageCodec->GetSocket().get_executor(), 1024),
      mScheduleChannel(mPackageCodec->GetSocket().get_executor(), 1024),
      mWatchdog(mPackageCodec->GetSocket().get_executor()),
      mExpiration(std::chrono::seconds(30)), mPlayerID(-1) {

    GetSocket().set_option(asio::ip::tcp::no_delay(true));
    GetSocket().set_option(asio::ip::tcp::socket::keep_alive(true));

    mKey = fmt::format("{}-{}", mPackageCodec->GetSocket().remote_endpoint().address().to_string(), utils::UnixTime());
}

UAgent::~UAgent() {
    Disconnect();
}

bool UAgent::IsSocketOpen() const {
    return GetSocket().is_open();
}

ATcpSocket &UAgent::GetSocket() const {
    return mPackageCodec->GetSocket();
}

asio::ip::address UAgent::RemoteAddress() const {
    if (IsSocketOpen()) {
        return GetSocket().remote_endpoint().address();
    }
    return {};
}

const std::string &UAgent::GetKey() const {
    return mKey;
}

void UAgent::SetExpireSecond(const int sec) {
    mExpiration = std::chrono::seconds(sec);
}

UServer *UAgent::GetServer() const {
    return mServer;
}

void UAgent::SetUpAgent(UServer *pServer) {
    mServer = pServer;
    mPackagePool = mServer->CreateUniquePackagePool();
    mPackagePool->Initial();
}

FPackageHandle UAgent::BuildPackage() const {
    if (!mPackagePool)
        throw std::runtime_error(std::format("{} - Package Pool Is Null Pointer", __FUNCTION__));

    return mPackagePool->Acquire<IPackage_Interface>();
}

void UAgent::ConnectToClient() {
    mReceiveTime = std::chrono::steady_clock::now();

    co_spawn(GetSocket().get_executor(), [self = shared_from_this()]() -> awaitable<void> {
        if (const auto ret = co_await self->mPackageCodec->Initial(); !ret) {
            self->Disconnect();
            co_return;
        }

        co_await (
            self->ReadPackage() ||
            self->WritePackage() ||
            self->Watchdog()
        );
    }, detached);
}

void UAgent::Disconnect() {
    if (!IsSocketOpen())
        return;

    GetSocket().close();
    mWatchdog.cancel();
    mPackageChannel.close();
    mScheduleChannel.close();

    // TODO: Do Logout Logic
    if (mPlayer != nullptr) {


        mServer->RemoveAgent(mPlayer->GetPlayerID());
    }
}

void UAgent::SetUpPlayer(unique_ptr<IPlayerBase> &&plr) {
    if (!IsSocketOpen())
        return;

    if (!mPlayer)
        throw std::runtime_error(std::format("{} - Other Player Already Exists", __FUNCTION__));

    mPlayer = std::move(plr);
    mPlayerID = mPlayer->GetPlayerID();

    co_spawn(GetSocket().get_executor(), [self = shared_from_this()]() -> awaitable<void> {

        // TODO: Player Login Logic

        co_await self->ProcessChannel();
    }, detached);
}

unique_ptr<IPlayerBase> UAgent::ExtractPlayer() {
    return std::move(mPlayer);
}


void UAgent::SendPackage(const FPackageHandle &pkg) {
    if (pkg == nullptr)
        return;

    if (const bool ret = mPackageChannel.try_send_via_dispatch(std::error_code{}, pkg); !ret) {
        co_spawn(GetSocket().get_executor(), [self = shared_from_this(), pkg]() -> awaitable<void> {
            co_await self->mPackageChannel.async_send(std::error_code{}, pkg);
        }, detached);
    }
}


awaitable<void> UAgent::WritePackage() {
    try {
        while (IsSocketOpen() && mPackageChannel.is_open()) {
            const auto [ec, pkg] = co_await mPackageChannel.async_receive();
            if (ec) {
                Disconnect();
                break;
            }

            if (pkg == nullptr)
                continue;

            if (const auto ret = co_await mPackageCodec->Encode(pkg.Get()); !ret) {
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

awaitable<void> UAgent::ReadPackage() {
    try {
        while (IsSocketOpen()) {
            const auto pkg = BuildPackage();
            if (pkg == nullptr)
                co_return;

            if (const auto ret = co_await mPackageCodec->Decode(pkg.Get()); !ret) {
                Disconnect();
                break;
            }

            const auto now = std::chrono::steady_clock::now();

            // Run The Login Branch
            if (mPlayerID < 0 && mPlayer == nullptr) {
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
                        login->OnLoginRequest(mKey, pkg);
                    }
                }

                mReceiveTime = now;
                continue;
            }

            // Update Receive Time Point For Watchdog
            mReceiveTime = now;
            mPlayer->OnPackage(pkg.Get());
        }
    } catch (const std::exception &e) {
        SPDLOG_ERROR("{:<20} - {}", __FUNCTION__, e.what());
        Disconnect();
    }
}

awaitable<void> UAgent::Watchdog() {
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

awaitable<void> UAgent::ProcessChannel() {
    try {
        while (IsSocketOpen() && mScheduleChannel.is_open() && mPlayer != nullptr) {
            const auto [ec, node] = co_await mScheduleChannel.async_receive();
            if (ec)
                break;

            if (node == nullptr)
                continue;

            if (mPlayer == nullptr)
                break;

            node->Execute(mPlayer.get());
        }
    } catch (const std::exception &e) {
        SPDLOG_ERROR("{:<20} - {}", __FUNCTION__, e.what());
    }
}
