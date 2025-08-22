#include "Agent.h"
#include "Server.h"
#include "Context.h"
#include "PlayerBase.h"
#include "Utils.h"
#include "base/AgentWorker.h"
#include "base/PackageCodec.h"
#include "login/LoginAuth.h"
#include "event/EventModule.h"

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
    : mServer(nullptr),
      mCodec(std::move(codec)),
      mOutput(mCodec->GetExecutor(), 1024),
      mChannel(mCodec->GetExecutor(), 1024),
      mWatchdog(mCodec->GetExecutor()),
      mExpiration(std::chrono::seconds(30)),
      mTimerManager(static_cast<asio::io_context &>(mCodec->GetExecutor().context())),
      bCachable(true) {

    GetSocket().set_option(asio::ip::tcp::no_delay(true));
    GetSocket().set_option(asio::ip::tcp::socket::keep_alive(true));

    mKey = fmt::format("{}-{}", RemoteAddress().to_string(), utils::UnixTime());
}

UAgent::~UAgent() {
    Disconnect();
}

bool UAgent::IsSocketOpen() const {
    return GetSocket().is_open();
}

ATcpSocket &UAgent::GetSocket() const {
    return mCodec->GetSocket();
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

int64_t UAgent::GetPlayerID() const {
    if (mPlayer == nullptr)
        return -1;

    return mPlayer->GetPlayerID();
}

void UAgent::SetExpireSecond(const int sec) {
    mExpiration = std::chrono::seconds(sec);
}

UServer *UAgent::GetServer() const {
    return mServer;
}

void UAgent::SetUpAgent(UServer *pServer) {
    mServer = pServer;

    mPool = mServer->CreateUniquePackagePool(static_cast<asio::io_context &>(mCodec->GetExecutor().context()));
    mPool->Initial();

    mWorker = mServer->CreateAgentWorker();
    mWorker->SetUpAgent(this);
}

FPackageHandle UAgent::BuildPackage() const {
    if (!mPool)
        throw std::runtime_error(std::format("{} - Package Pool Is Null Pointer", __FUNCTION__));

    return mPool->Acquire<IPackage_Interface>();
}

void UAgent::ConnectToClient() {
    mReceiveTime = std::chrono::steady_clock::now();

    co_spawn(GetSocket().get_executor(), [self = shared_from_this()]() -> awaitable<void> {
        if (const auto ret = co_await self->mCodec->Initial(); !ret) {
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

    mWatchdog.cancel();
    mOutput.close();
    mChannel.close();
}

void UAgent::SetUpPlayer(FPlayerHandle &&plr) {
    if (!IsSocketOpen())
        return;

    if (!mPlayer)
        throw std::runtime_error(std::format("{} - Other Player Already Exists", __FUNCTION__));

    mPlayer = std::move(plr);
    mReceiveTime = std::chrono::steady_clock::now();

    co_spawn(GetSocket().get_executor(), [self = shared_from_this()]() -> awaitable<void> {
        const auto pkg = self->mWorker->OnLoginSuccess(self->mPlayer->GetPlayerID());

        pkg->SetPackageID(LOGIN_RESPONSE_PACKAGE_ID);
        pkg->SetSource(SERVER_SOURCE_ID);
        pkg->SetTarget(CLIENT_TARGET_ID);

        self->SendPackage(pkg);

        self->mPlayer->OnLogin();
        co_await self->ProcessChannel();
    }, detached);
}

FPlayerHandle UAgent::ExtractPlayer() {
    return std::move(mPlayer);
}

void UAgent::PushPackage(const FPackageHandle &pkg) {
    if (!mChannel.is_open())
        return;

    if (pkg == nullptr)
        return;

    auto node = make_unique<UPackageNode>();
    node->SetPackage(pkg);

    if (const auto ret = mChannel.try_send_via_dispatch(std::error_code{}, std::move(node)); !ret) {
        auto temp = make_unique<UPackageNode>();
        temp->SetPackage(pkg);

        co_spawn(GetSocket().get_executor(), [self = shared_from_this(), node = std::move(temp)]() mutable -> awaitable<void> {
            co_await self->mChannel.async_send(std::error_code{}, std::move(node));
        }, detached);
    }
}

void UAgent::PushEvent(const std::shared_ptr<IEventParam_Interface> &event) {
    if (!mChannel.is_open())
        return;

    if (event == nullptr)
        return;

    auto node = make_unique<UEventNode>();
    node->SetEventParam(event);

    if (const auto ret = mChannel.try_send_via_dispatch(std::error_code{}, std::move(node)); !ret) {
        auto temp = make_unique<UEventNode>();
        temp->SetEventParam(event);

        co_spawn(GetSocket().get_executor(), [self = shared_from_this(), node = std::move(temp)]() mutable -> awaitable<void> {
            co_await self->mChannel.async_send(std::error_code{}, std::move(node));
        }, detached);
    }
}

void UAgent::PushTask(const APlayerTask &task) {
    if (!mChannel.is_open())
        return;

    if (task == nullptr)
        return;

    auto node = make_unique<UTaskNode>();
    node->SetTask(task);

    if (const auto ret = mChannel.try_send_via_dispatch(std::error_code{}, std::move(node)); !ret) {
        auto temp = make_unique<UTaskNode>();
        temp->SetTask(task);

        co_spawn(GetSocket().get_executor(), [self = shared_from_this(), node = std::move(temp)]() mutable -> awaitable<void> {
            co_await self->mChannel.async_send(std::error_code{}, std::move(node));
        }, detached);
    }
}

void UAgent::PostPackage(const FPackageHandle &pkg) const {
    if (pkg == nullptr)
        return;

    const auto target = pkg->GetTarget();
    if (target <= 0)
        return;

    if (const auto context = GetServer()->FindService(target)) {
        SPDLOG_TRACE("{:<20} - From Player[ID: {}, Addr: {}] To Service[ID: {}, Name: {}]",
            __FUNCTION__, mPlayer->GetPlayerID(), RemoteAddress().to_string(), target, context->GetServiceName());

        pkg->SetSource(PLAYER_TARGET_ID);
        context->PushPackage(pkg);
    }
}

void UAgent::PostPackage(const std::string &name, const FPackageHandle &pkg) const {
    if (pkg == nullptr)
        return;

    if (const auto context = GetServer()->FindService(name)) {
        const auto target = context->GetServiceID();

        SPDLOG_TRACE("{:<20} - From Player[ID: {}, Addr: {}] To Service[ID: {}, Name: {}]",
            __FUNCTION__, mPlayer->GetPlayerID(), RemoteAddress().to_string(),
            static_cast<int>(target), context->GetServiceName());

        pkg->SetSource(PLAYER_TARGET_ID);
        pkg->SetTarget(target);

        context->PushPackage(pkg);
    }
}

void UAgent::PostTask(const int64_t target, const AServiceTask &task) const {
    if (task == nullptr)
        return;

    if (target <= 0)
        return;

    if (const auto context = GetServer()->FindService(target)) {
        SPDLOG_TRACE("{:<20} - From Player[ID: {}, Addr: {}] To Service[ID: {}, Name: {}]",
            __FUNCTION__, mPlayer->GetPlayerID(), RemoteAddress().to_string(),
            target, context->GetServiceName());

        context->PushTask(task);
    }
}

void UAgent::PostTask(const std::string &name, const AServiceTask &task) const {
    if (task == nullptr)
        return;

    if (name.empty())
        return;

    if (const auto context = GetServer()->FindService(name)) {
        const auto target = context->GetServiceID();

        SPDLOG_TRACE("{:<20} - From Player[ID: {}, Addr: {}] To Service[ID: {}, Name: {}]",
            __FUNCTION__, mPlayer->GetPlayerID(), RemoteAddress().to_string(),
            static_cast<int>(target), context->GetServiceName());

        context->PushTask(task);
    }
}

FTimerHandle UAgent::CreateTimer(const ATimerTask &task, const int delay, const int rate) {
    if (mPlayer == nullptr || !mChannel.is_open())
        return {};

    return mTimerManager.CreateTimer(task, delay, rate);
}

void UAgent::CancelTimer(const int64_t tid) const {
    mTimerManager.CancelTimer(tid);
}

void UAgent::CancelAllTimers() {
    mTimerManager.CancelAll();
}

void UAgent::ListenEvent(const int event) {
    if (mPlayer == nullptr || !mChannel.is_open())
        return;

    auto *module = GetServer()->GetModule<UEventModule>();
    if (module == nullptr)
        return;

    module->PlayerListenEvent(GenerateHandle(), event);
}

void UAgent::RemoveListener(const int event) {
    auto *module = GetServer()->GetModule<UEventModule>();
    if (module == nullptr)
        return;

    module->RemovePlayerListenerByEvent(GenerateHandle(), event);
}

void UAgent::DispatchEvent(const shared_ptr<IEventParam_Interface> &param) const {
    if (mPlayer == nullptr || !mChannel.is_open())
        return;

    auto *module = GetServer()->GetModule<UEventModule>();
    if (module == nullptr)
        return;

    module->Dispatch(param);
}

void UAgent::SendPackage(const FPackageHandle &pkg) {
    if (pkg == nullptr)
        return;

    if (const bool ret = mOutput.try_send_via_dispatch(std::error_code{}, pkg); !ret && mOutput.is_open()) {
        co_spawn(GetSocket().get_executor(), [self = shared_from_this(), pkg]() -> awaitable<void> {
            co_await self->mOutput.async_send(std::error_code{}, pkg);
        }, detached);
    }
}

void UAgent::OnLoginFailed(const std::string &desc) {
    if (mWorker != nullptr)
        throw std::logic_error(std::format("{} - Handler Is Null Pointer", __FUNCTION__));

    bCachable = false;

    const auto pkg = mWorker->OnLoginFailure(desc);
    if (pkg == nullptr) {
        Disconnect();
        return;
    }

    pkg->SetPackageID(LOGIN_FAILED_PACKAGE_ID);
    pkg->SetSource(SERVER_SOURCE_ID);
    pkg->SetTarget(CLIENT_TARGET_ID);

    SendPackage(pkg);
}

void UAgent::OnRepeated(const std::string &addr) {
    if (mWorker != nullptr)
        throw std::logic_error(std::format("{} - Handler Is Null Pointer", __FUNCTION__));

    const auto pkg = mWorker->OnRepeated(addr);
    if (pkg == nullptr) {
        Disconnect();
        return;
    }

    pkg->SetPackageID(LOGIN_REPEATED_PACKAGE_ID);
    pkg->SetSource(SERVER_SOURCE_ID);
    pkg->SetTarget(CLIENT_TARGET_ID);

    SendPackage(pkg);
}

FAgentHandle UAgent::GenerateHandle() {
    if (mPlayer == nullptr || !mChannel.is_open())
        return {};

    return FAgentHandle(mPlayer->GetPlayerID(), weak_from_this());
}


awaitable<void> UAgent::WritePackage() {
    try {
        while (IsSocketOpen() && mOutput.is_open()) {
            const auto [ec, pkg] = co_await mOutput.async_receive();
            if (ec) {
                Disconnect();
                break;
            }

            if (pkg == nullptr)
                continue;

            if (const auto ret = co_await mCodec->Encode(pkg.Get()); !ret) {
                Disconnect();
                break;
            }

            // Can Do Something Here
            if (pkg->GetPackageID() == LOGIN_FAILED_PACKAGE_ID ||
                pkg->GetPackageID() == LOGIN_REPEATED_PACKAGE_ID) {
                Disconnect();
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

            if (const auto ret = co_await mCodec->Decode(pkg.Get()); !ret) {
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

            switch (pkg->GetPackageID()) {
                case LOGIN_REQUEST_PACKAGE_ID:
                case HEARTBEAT_PACKAGE_ID: break;
                case PLATFORM_PACKAGE_ID: {
                    // TODO: Parse Platform Info
                } break;
                case LOGOUT_REQUEST_PACKAGE_ID: {
                    // TODO: Parse Logout Request
                    bCachable = false;
                } break;
                default: {
                    if (const auto target = pkg->GetTarget(); target == PLAYER_TARGET_ID) {
                        mPlayer->OnPackage(pkg.Get());
                    } else if (target > 0) {
                        // Post Package To Service
                        PostPackage(pkg);
                    }
                }
            }
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
        while (IsSocketOpen() && mChannel.is_open() && mPlayer != nullptr) {
            const auto [ec, node] = co_await mChannel.async_receive();
            if (ec || !mChannel.is_open())
                break;

            if (node == nullptr)
                continue;

            if (mPlayer == nullptr)
                break;

            node->Execute(mPlayer.Get());
        }
    } catch (const std::exception &e) {
        SPDLOG_ERROR("{:<20} - {}", __FUNCTION__, e.what());
    }

    CleanUp();
}

void UAgent::CleanUp() {
    if (mPlayer != nullptr) {
        mPlayer->OnLogout();
        mPlayer->Save();

        if (auto *module = GetServer()->GetModule<UEventModule>()) {
            module->RemovePlayerListener(GenerateHandle());
        }

        if (bCachable) {
            mServer->RecyclePlayer(std::move(mPlayer));
        } else {
            mServer->RemovePlayer(mPlayer->GetPlayerID());
        }
    }

    mServer->RemoveAgent(mKey);
}
