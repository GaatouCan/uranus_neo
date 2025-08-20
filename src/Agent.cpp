#include "Agent.h"
#include "Server.h"
#include "PlayerBase.h"
#include "Utils.h"
#include "base/Recycler.h"
#include "base/AgentHandler.h"
#include "login/LoginAuth.h"
#include "base/PackageCodec.h"

#include <asio/experimental/awaitable_operators.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>


using namespace asio::experimental::awaitable_operators;
using namespace std::literals::chrono_literals;


void UAgent::UPackageNode::SetPackage(const FPackageHandle &package) {
    mPackage = package;
}

void UAgent::UPackageNode::Execute(IPlayerBase *pPlayer) const {
    if (pPlayer != nullptr && mPackage != nullptr) {
        pPlayer->OnPackage(mPackage.Get());
    }
}

void UAgent::UEventNode::SetEventParam(const std::shared_ptr<IEventParam_Interface> &event) {
    mEvent = event;
}

void UAgent::UEventNode::Execute(IPlayerBase *pPlayer) const {
    if (pPlayer != nullptr && mEvent != nullptr) {
        pPlayer->OnEvent(mEvent.get());
    }
}

void UAgent::UTaskNode::SetTask(const APlayerTask &task) {
    mTask = task;
}

void UAgent::UTaskNode::Execute(IPlayerBase *pPlayer) const {
    if (pPlayer != nullptr && mTask != nullptr) {
        std::invoke(mTask, pPlayer);
    }
}

UAgent::UAgent(unique_ptr<IPackageCodec_Interface> &&codec)
    : mPackageCodec(std::move(codec)),
      mPackageChannel(mPackageCodec->GetSocket().get_executor(), 1024),
      mScheduleChannel(mPackageCodec->GetSocket().get_executor(), 1024),
      mWatchdog(mPackageCodec->GetSocket().get_executor()),
      mExpiration(std::chrono::seconds(30)) {

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

    mHandler = mServer->CreateAgentHandler();
    mHandler->SetUpAgent(this);
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

    if (mPlayer != nullptr) {
        mPlayer->OnLogout();
        mPlayer->Save();
        mServer->RemoveAgent(std::move(mPlayer));
    }
}

void UAgent::SetUpPlayer(unique_ptr<IPlayerBase> &&plr) {
    if (!IsSocketOpen())
        return;

    if (!mPlayer)
        throw std::runtime_error(std::format("{} - Other Player Already Exists", __FUNCTION__));

    mPlayer = std::move(plr);
    mReceiveTime = std::chrono::steady_clock::now();

    co_spawn(GetSocket().get_executor(), [self = shared_from_this()]() -> awaitable<void> {
        const auto pkg = self->mHandler->OnLoginSuccess(self->mPlayer->GetPlayerID());

        pkg->SetPackageID(LOGIN_RESPONSE_PACKAGE_ID);
        pkg->SetSource(SERVER_SOURCE_ID);
        pkg->SetTarget(CLIENT_TARGET_ID);

        self->SendPackage(pkg);

        self->mPlayer->OnLogin();
        co_await self->ProcessChannel();
    }, detached);
}

unique_ptr<IPlayerBase> UAgent::ExtractPlayer() {
    return std::move(mPlayer);
}

void UAgent::PushPackage(const FPackageHandle &pkg) {
    if (!mScheduleChannel.is_open())
        return;

    if (pkg == nullptr)
        return;

    auto node = make_unique<UPackageNode>();
    node->SetPackage(pkg);

    if (const auto ret = mScheduleChannel.try_send_via_dispatch(std::error_code{}, std::move(node)); !ret) {
        auto temp = make_unique<UPackageNode>();
        temp->SetPackage(pkg);

        co_spawn(GetSocket().get_executor(), [self = shared_from_this(), node = std::move(temp)]() mutable -> awaitable<void> {
            co_await self->mScheduleChannel.async_send(std::error_code{}, std::move(node));
        }, detached);
    }
}

void UAgent::PushEvent(const std::shared_ptr<IEventParam_Interface> &event) {
    if (!mScheduleChannel.is_open())
        return;

    if (event == nullptr)
        return;

    auto node = make_unique<UEventNode>();
    node->SetEventParam(event);

    if (const auto ret = mScheduleChannel.try_send_via_dispatch(std::error_code{}, std::move(node)); !ret) {
        auto temp = make_unique<UEventNode>();
        temp->SetEventParam(event);

        co_spawn(GetSocket().get_executor(), [self = shared_from_this(), node = std::move(temp)]() mutable -> awaitable<void> {
            co_await self->mScheduleChannel.async_send(std::error_code{}, std::move(node));
        }, detached);
    }
}

void UAgent::PushTask(const APlayerTask &task) {
    if (!mScheduleChannel.is_open())
        return;

    if (task == nullptr)
        return;

    auto node = make_unique<UTaskNode>();
    node->SetTask(task);

    if (const auto ret = mScheduleChannel.try_send_via_dispatch(std::error_code{}, std::move(node)); !ret) {
        auto temp = make_unique<UTaskNode>();
        temp->SetTask(task);

        co_spawn(GetSocket().get_executor(), [self = shared_from_this(), node = std::move(temp)]() mutable -> awaitable<void> {
            co_await self->mScheduleChannel.async_send(std::error_code{}, std::move(node));
        }, detached);
    }
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

void UAgent::OnLoginFailed(const std::string &desc) {
    if (mHandler != nullptr)
        throw std::logic_error(std::format("{} - Handler Is Null Pointer", __FUNCTION__));

    const auto pkg = mHandler->OnLoginFailure(desc);
    if (pkg == nullptr) {
        this->Disconnect();
        return;
    }

    pkg->SetPackageID(LOGIN_FAILED_PACKAGE_ID);
    pkg->SetSource(SERVER_SOURCE_ID);
    pkg->SetTarget(CLIENT_TARGET_ID);

    this->SendPackage(pkg);
}

void UAgent::OnRepeat(const std::string &addr) {
    if (mHandler != nullptr)
        throw std::logic_error(std::format("{} - Handler Is Null Pointer", __FUNCTION__));

    const auto pkg = mHandler->OnRepeated(addr);
    if (pkg == nullptr) {
        this->Disconnect();
        return;
    }

    pkg->SetPackageID(LOGIN_REPEATED_PACKAGE_ID);
    pkg->SetSource(SERVER_SOURCE_ID);
    pkg->SetTarget(CLIENT_TARGET_ID);

    this->SendPackage(pkg);
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
            if (pkg->GetPackageID() == LOGIN_FAILED_PACKAGE_ID ||
                pkg->GetPackageID() == LOGIN_REPEATED_PACKAGE_ID) {
                this->Disconnect();
                break;
            }
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
            if (mPlayer == nullptr) {
                // Handle Login Logic
                if (auto *login = GetServer()->GetModule<ULoginAuth>(); login != nullptr) {
                    login->OnLoginRequest(mKey, pkg);
                }
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
