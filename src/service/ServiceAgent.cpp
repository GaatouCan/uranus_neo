#include "ServiceAgent.h"
#include "ServiceBase.h"
#include "Server.h"
#include "ServiceModule.h"
#include "gateway/PlayerAgent.h"
#include "base/Package.h"
#include "base/DataAsset.h"
#include "event/EventModule.h"
#include "route/RouteModule.h"

#include <spdlog/spdlog.h>


using AServiceCreator = IServiceBase *(*)();
using AServiceDestroyer = void (*)(IServiceBase *);


UTickerNode::UTickerNode()
    : mDeltaTime(0) {
}

void UTickerNode::SetCurrentTickTime(const ASteadyTimePoint timepoint) {
    mTickTime = timepoint;
}

void UTickerNode::SetDeltaTime(const ASteadyDuration delta) {
    mDeltaTime = delta;
}

void UTickerNode::Execute(IActorBase *pActor) const {
    if (pActor == nullptr)
        return;

    if (auto *pService = dynamic_cast<IServiceBase *>(pActor)) {
        pService->OnUpdate(mTickTime, mDeltaTime);
    }
}

UServiceAgent::UServiceAgent(asio::io_context &ctx)
    : IAgentBase(ctx),
      mServiceID(INVALID_SERVICE_ID),
      mService(nullptr) {
}

UServiceAgent::~UServiceAgent() {
    Stop();
}

void UServiceAgent::SetUpServiceID(const int64_t sid) {
    mServiceID = sid;
}

void UServiceAgent::SetUpLibrary(const FSharedLibrary &library) {
    mLibrary = library;
}

bool UServiceAgent::Initial(IModuleBase *pModule, IDataAsset_Interface *pData) {
    auto *module = dynamic_cast<UServiceModule *>(pModule);
    if (module == nullptr)
        throw std::bad_cast();

    if (!mLibrary.IsValid())
        throw std::logic_error(std::format("{} - Shared Library Is Null", __FUNCTION__));

    mModule = module;

    // Start To Create Service

    // Load The Constructor From Shared Library
    auto creator = mLibrary.GetSymbol<AServiceCreator>("CreateInstance");
    if (creator == nullptr) {
        SPDLOG_ERROR("{:<20} - Can't Load Creator", __FUNCTION__);
        return false;
    }

    mService = std::invoke(creator);
    if (mService == nullptr) {
        SPDLOG_ERROR("{:<20} - Can't Create Service", __FUNCTION__);
        return false;
    }

    mChannel = make_unique<AChannel>(mContext, 1024);

    // Create Package Pool For Data Exchange
    mPackagePool = module->GetServer()->CreateUniquePackagePool(mContext);
    mPackagePool->Initial();

    // Initial Service
    mService->SetUpAgent(this);
    const auto ret = mService->Initial(pData);

    // Context And Service Initialized
    SPDLOG_TRACE("{:<20} - Context[{:p}] Service[{}] Initial Successfully",
        __FUNCTION__, static_cast<const void *>(this), mService->GetServiceName());

    // Delete The Data Asset For Initialization
    delete pData;

    return ret;
}

void UServiceAgent::Stop() {
    if (mChannel != nullptr || mChannel->is_open()) {
        mChannel->close();
    }
}

bool UServiceAgent::BootService() {
    if (mModule == nullptr || mChannel == nullptr || mPackagePool == nullptr || !mLibrary.IsValid() || mService == nullptr)
        throw std::logic_error(std::format("{:<20} - Not Initialized", __FUNCTION__));

    if (const auto res = mService->Start(); !res) {
        SPDLOG_ERROR("{:<20} - Context[{:p}], Service[{} - {}] Failed To Boot.",
            __FUNCTION__, static_cast<const void *>(this), static_cast<int64_t>(mServiceID), GetServiceName());

        return false;
    }

    SPDLOG_TRACE("{:<20} - Context[{:p}], Service[{} - {}] Started.",
        __FUNCTION__, static_cast<const void *>(this), mServiceID, GetServiceName());

    if (mService->bUpdatePerTick) {
        if (auto *module = dynamic_cast<UServiceModule *>(mModule)) {
            module->AddTicker(mServiceID);
        }
    }

    co_spawn(mContext, [self = SharedFromThis()] -> awaitable<void> {
        co_await self->ProcessChannel();
    }, detached);

    return true;
}

std::string UServiceAgent::GetServiceName() const {
    if (mService)
        return mService->GetServiceName();
    return "UNKNOWN";
}

int64_t UServiceAgent::GetServiceID() const {
    return mServiceID;
}

void UServiceAgent::PushTicker(const ASteadyTimePoint timepoint, const ASteadyDuration delta) {
    if (mService == nullptr || mChannel == nullptr || !mChannel->is_open())
        return;

    // SPDLOG_TRACE("{:<20} - Context[{:p}], Service[{}]",
    //              __FUNCTION__, static_cast<const void *>(this), GetServiceName());

    auto node = make_unique<UTickerNode>();
    node->SetCurrentTickTime(timepoint);
    node->SetDeltaTime(delta);

    if (const bool ret = mChannel->try_send_via_dispatch(std::error_code{}, std::move(node)); !ret && mChannel->is_open()) {
        auto temp = make_unique<UTickerNode>();
        temp->SetCurrentTickTime(timepoint);
        temp->SetDeltaTime(delta);

        co_spawn(mContext, [self = SharedFromThis(), temp = std::move(temp)]() mutable -> awaitable<void> {
            co_await self->mChannel->async_send(std::error_code{}, std::move(temp));
        }, detached);
    }
}

void UServiceAgent::PostPackage(const FPackageHandle &pkg) const {
    if (pkg == nullptr)
        return;

    if (const auto target = pkg->GetTarget(); target < 0 || target == mServiceID)
        return;

    const auto *router = GetServer()->GetModule<URouteModule>();
    if (!router)
        return;

    router->PostPackage(pkg);
}

void UServiceAgent::PostPackage(const std::string &name, const FPackageHandle &pkg) const {
    if (pkg == nullptr)
        return;

    if (name.empty() || name == GetServiceName())
        return;

    const auto *router = GetServer()->GetModule<URouteModule>();
    if (!router)
        return;

    router->PostPackage(name, pkg);
}

void UServiceAgent::PostTask(const int64_t target, const AActorTask &task) const {
    if (task == nullptr)
        return;

    if (target < 0 || target == mServiceID)
        return;

    const auto *router = GetServer()->GetModule<URouteModule>();
    if (!router)
        return;

    router->PostTask(true, target, task);
}

void UServiceAgent::PostTask(const std::string &name, const AActorTask &task) const {
    if (task == nullptr)
        return;

    if (name.empty() || name == GetServiceName())
        return;

    const auto *router = GetServer()->GetModule<URouteModule>();
    if (!router)
        return;

    router->PostTask(name, task);
}

void UServiceAgent::SendToPlayer(const int64_t pid, const FPackageHandle &pkg) const {
    if (pkg == nullptr)
        return;

    if (pid < 0)
        return;

    const auto *router = GetServer()->GetModule<URouteModule>();
    if (!router)
        return;

    pkg->SetTarget(PLAYER_TARGET_ID);
    router->PostPackage(pkg, pid);
}

void UServiceAgent::SendToClient(const int64_t pid, const FPackageHandle &pkg) const {
    if (pkg == nullptr)
        return;

    if (pid <= 0)
        return;

    const auto *router = GetServer()->GetModule<URouteModule>();
    if (!router)
        return;

    pkg->SetTarget(CLIENT_TARGET_ID);
    router->SendToClient(pid, pkg);
}

void UServiceAgent::PostToPlayer(const int64_t pid, const AActorTask &task) const {
    if (task == nullptr)
        return;

    if (pid <= 0)
        return;

    const auto *router = GetServer()->GetModule<URouteModule>();
    if (!router)
        return;

    router->PostTask(false, pid, task);
}

void UServiceAgent::ListenEvent(const int event) {
    if (mService == nullptr || mChannel == nullptr || !mChannel->is_open())
        return;

    auto *module = GetServer()->GetModule<UEventModule>();
    if (module == nullptr)
        return;

    module->ServiceListenEvent(mServiceID, WeakFromThis(), event);
}

void UServiceAgent::RemoveListener(const int event) const {
    auto *module = GetServer()->GetModule<UEventModule>();
    if (module == nullptr)
        return;

    module->RemoveServiceListenerByEvent(mServiceID, event);
}

void UServiceAgent::DispatchEvent(const shared_ptr<IEventParam_Interface> &param) const {
    if (mService == nullptr || mChannel == nullptr || !mChannel->is_open())
        return;

    auto *module = GetServer()->GetModule<UEventModule>();
    if (module == nullptr)
        return;

    module->Dispatch(param);
}

IActorBase *UServiceAgent::GetActor() const {
    return mService;
}


void UServiceAgent::CleanUp() {
    if (!mLibrary.IsValid())
        return;

    if (mService == nullptr)
        return;

    mService->Stop();
    auto destroyer = mLibrary.GetSymbol<AServiceDestroyer>("DestroyInstance");
    if (destroyer != nullptr) {
        std::invoke(destroyer, mService);
    }

    mService = nullptr;
    mLibrary.Reset();

    mServiceID = INVALID_SERVICE_ID;
}

shared_ptr<UServiceAgent> UServiceAgent::SharedFromThis() {
    return std::dynamic_pointer_cast<UServiceAgent>(shared_from_this());
}

weak_ptr<UServiceAgent> UServiceAgent::WeakFromThis() {
    return this->SharedFromThis();
}
