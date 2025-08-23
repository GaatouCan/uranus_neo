#include "PlayerAgent.h"
#include "PlayerBase.h"
#include "Gateway.h"
#include "Server.h"
#include "Utils.h"
#include "base/AgentHandler.h"
#include "base/PackageCodec.h"
#include "login/LoginAuth.h"
#include "event/EventModule.h"
#include "service/ServiceAgent.h"
#include "route/RouteModule.h"

#include <asio/experimental/awaitable_operators.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>


using namespace asio::experimental::awaitable_operators;
using namespace std::literals::chrono_literals;


UPlayerAgent::UPlayerAgent(unique_ptr<IPackageCodec_Interface> &&codec)
    : IAgentBase(static_cast<asio::io_context &>(codec->GetExecutor().context())),
      mCodec(std::move(codec)),
      mOutput(mContext, 1024),
      mWatchdog(mContext),
      mExpiration(std::chrono::seconds(30)),
      bCachable(true) {

    GetSocket().set_option(asio::ip::tcp::no_delay(true));
    GetSocket().set_option(asio::ip::tcp::socket::keep_alive(true));

    mKey = fmt::format("{}-{}", RemoteAddress().to_string(), utils::UnixTime());
}

UPlayerAgent::~UPlayerAgent() {
    Disconnect();
}

bool UPlayerAgent::IsSocketOpen() const {
    return GetSocket().is_open();
}

ATcpSocket &UPlayerAgent::GetSocket() const {
    return mCodec->GetSocket();
}

asio::ip::address UPlayerAgent::RemoteAddress() const {
    if (IsSocketOpen()) {
        return GetSocket().remote_endpoint().address();
    }
    return {};
}

const std::string &UPlayerAgent::GetKey() const {
    return mKey;
}

int64_t UPlayerAgent::GetPlayerID() const {
    if (mPlayer == nullptr)
        return -1;

    return mPlayer->GetPlayerID();
}

void UPlayerAgent::SetExpireSecond(const int sec) {
    mExpiration = std::chrono::seconds(sec);
}


bool UPlayerAgent::Initial(IModuleBase *pModule, IDataAsset_Interface *pData) {
    auto *gateway = dynamic_cast<UGateway *>(pModule);
    if (gateway == nullptr)
        throw std::bad_cast();

    mModule = gateway;

    // Create The Channel
    mChannel = make_unique<AChannel>(mContext, 1024);

    // Create The Package Pool
    mPackagePool = gateway->GetServer()->CreateUniquePackagePool(mContext);
    mPackagePool->Initial();

    mHandler = gateway->CreateAgentHandler();
    mHandler->SetUpAgent(this);

    return true;
}

void UPlayerAgent::ConnectToClient() {
    mReceiveTime = std::chrono::steady_clock::now();

    co_spawn(mContext, [self = SharedFromThis()]() -> awaitable<void> {
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

void UPlayerAgent::Disconnect() {
    if (IsSocketOpen()) {
        GetSocket().close();
    }

    if (mChannel != nullptr || mChannel->is_open()) {
        mChannel->close();
    }

    mWatchdog.cancel();
    mOutput.close();
}

void UPlayerAgent::SetUpPlayer(FPlayerHandle &&plr) {
    if (!IsSocketOpen())
        return;

    if (!mPlayer)
        throw std::runtime_error(std::format("{} - Other Player Already Exists", __FUNCTION__));

    mPlayer = std::move(plr);
    mReceiveTime = std::chrono::steady_clock::now();

    co_spawn(mContext, [self = SharedFromThis()]() -> awaitable<void> {
        const auto pkg = self->mHandler->OnLoginSuccess(self->mPlayer->GetPlayerID());

        pkg->SetPackageID(LOGIN_RESPONSE_PACKAGE_ID);
        pkg->SetSource(SERVER_SOURCE_ID);
        pkg->SetTarget(CLIENT_TARGET_ID);

        self->SendPackage(pkg);

        self->mPlayer->Initial();
        co_await self->ProcessChannel();
    }, detached);
}

FPlayerHandle UPlayerAgent::ExtractPlayer() {
    return std::move(mPlayer);
}

void UPlayerAgent::PostPackage(const FPackageHandle &pkg) const {
    if (pkg == nullptr)
        return;

    const auto *router = GetServer()->GetModule<URouteModule>();
    if (!router)
        return;

    pkg->SetSource(PLAYER_TARGET_ID);
    router->PostPackage(pkg);
}

void UPlayerAgent::PostPackage(const std::string &name, const FPackageHandle &pkg) const {
    if (pkg == nullptr)
        return;

    const auto *router = GetServer()->GetModule<URouteModule>();
    if (!router)
        return;

    pkg->SetSource(PLAYER_TARGET_ID);
    router->PostPackage(name, pkg);
}

void UPlayerAgent::PostTask(const int64_t target, const AActorTask &task) const {
    if (target < 0 || task == nullptr)
        return;

    const auto *router = GetServer()->GetModule<URouteModule>();
    if (!router)
        return;

    router->PostTask(true, target, task);
}

void UPlayerAgent::PostTask(const std::string &name, const AActorTask &task) const {
    if (name.empty() || task == nullptr)
        return;

    const auto *router = GetServer()->GetModule<URouteModule>();
    if (!router)
        return;

    router->PostTask(name, task);
}

void UPlayerAgent::ListenEvent(const int event) {
    if (mPlayer == nullptr || mHandler == nullptr || !mChannel->is_open())
        return;

    auto *module = GetServer()->GetModule<UEventModule>();
    if (module == nullptr)
        return;

    module->PlayerListenEvent(mPlayer->GetPlayerID(), WeakFromThis(), event);
}

void UPlayerAgent::RemoveListener(const int event) const {
    auto *module = GetServer()->GetModule<UEventModule>();
    if (module == nullptr)
        return;

    module->RemovePlayerListenerByEvent(mPlayer->GetPlayerID(), event);
}

void UPlayerAgent::DispatchEvent(const shared_ptr<IEventParam_Interface> &param) const {
    if (mPlayer == nullptr || mHandler == nullptr || !mChannel->is_open())
        return;

    auto *module = GetServer()->GetModule<UEventModule>();
    if (module == nullptr)
        return;

    module->Dispatch(param);
}

void UPlayerAgent::SendPackage(const FPackageHandle &pkg) {
    if (pkg == nullptr)
        return;

    if (const bool ret = mOutput.try_send_via_dispatch(std::error_code{}, pkg); !ret && mOutput.is_open()) {
        co_spawn(mContext, [self = SharedFromThis(), pkg]() -> awaitable<void> {
            co_await self->mOutput.async_send(std::error_code{}, pkg);
        }, detached);
    }
}

void UPlayerAgent::OnLoginFailed(const int code, const std::string &desc) {
    if (mHandler != nullptr)
        throw std::logic_error(std::format("{} - Handler Is Null Pointer", __FUNCTION__));

    bCachable = false;

    const auto pkg = mHandler->OnLoginFailure(code, desc);
    if (pkg == nullptr) {
        Disconnect();
        return;
    }

    pkg->SetPackageID(LOGIN_FAILED_PACKAGE_ID);
    pkg->SetSource(SERVER_SOURCE_ID);
    pkg->SetTarget(CLIENT_TARGET_ID);

    SendPackage(pkg);
}

void UPlayerAgent::OnRepeated(const std::string &addr) {
    if (mHandler != nullptr)
        throw std::logic_error(std::format("{} - Handler Is Null Pointer", __FUNCTION__));

    const auto pkg = mHandler->OnRepeated(addr);
    if (pkg == nullptr) {
        Disconnect();
        return;
    }

    pkg->SetPackageID(LOGIN_REPEATED_PACKAGE_ID);
    pkg->SetSource(SERVER_SOURCE_ID);
    pkg->SetTarget(CLIENT_TARGET_ID);

    SendPackage(pkg);
}

IActorBase *UPlayerAgent::GetActor() const {
    if (mPlayer != nullptr)
        return mPlayer.Get();

    return nullptr;
}

shared_ptr<UPlayerAgent> UPlayerAgent::SharedFromThis() {
    return std::dynamic_pointer_cast<UPlayerAgent>(shared_from_this());
}

weak_ptr<UPlayerAgent> UPlayerAgent::WeakFromThis() {
    return this->SharedFromThis();
}

awaitable<void> UPlayerAgent::WritePackage() {
    try {
        while (IsSocketOpen() && mOutput.is_open()) {
            const auto [ec, pkg] = co_await mOutput.async_receive();
            if (ec) {
                Disconnect();
                break;
            }

            if (pkg == nullptr || pkg->GetTarget() != CLIENT_TARGET_ID)
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

awaitable<void> UPlayerAgent::ReadPackage() {
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

awaitable<void> UPlayerAgent::Watchdog() {
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

void UPlayerAgent::CleanUp() {
    auto *gateway = dynamic_cast<UGateway *>(mModule);
    if (gateway == nullptr)
        return;

    if (mPlayer != nullptr) {
        mPlayer->OnLogout();
        mPlayer->Save();

        if (bCachable) {
            gateway->RecyclePlayer(std::move(mPlayer));
        } else {
            gateway->RemovePlayer(mPlayer->GetPlayerID());
        }
    }

    gateway->RemoveAgent(mKey);
}
