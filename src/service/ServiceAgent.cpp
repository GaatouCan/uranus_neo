#include "ServiceAgent.h"
#include "ServiceBase.h"
#include "Server.h"
#include "ServiceModule.h"
#include "base/Package.h"
#include "base/DataAsset.h"
#include "gateway/PlayerAgent.h"
#include "event/EventModule.h"
#include "route/RouteModule.h"

#include <spdlog/spdlog.h>


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

    // Only Service SubClass Can Update
    if (auto *pService = dynamic_cast<IServiceBase *>(pActor)) {
        pService->OnUpdate(mTickTime, mDeltaTime);
    }
}

UServiceAgent::UServiceAgent(asio::io_context &ctx)
    : IAgentBase(ctx, SERVICE_CHANNEL_SIZE),
      mServiceID(INVALID_SERVICE_ID) {
}

UServiceAgent::~UServiceAgent() {
    Stop();
}

void UServiceAgent::SetUpServiceID(const int64_t sid) {
    mServiceID = sid;
}

void UServiceAgent::SetUpService(FServiceHandle &&service) {
    mService = std::move(service);
}


bool UServiceAgent::Initial(IModuleBase *pModule, IDataAsset_Interface *pData) {
    // Check If The Shared Library Assigned
    if (!mService)
        throw std::logic_error(std::format("{} - Shared Library Is Null", __FUNCTION__));

    // Assign The mModule
    auto *module = dynamic_cast<UServiceModule *>(pModule);
    if (module == nullptr)
        throw std::bad_cast();

    mModule = module;

    // Create Package Pool For Data Exchange
    mPackagePool = module->GetServer()->CreateUniquePackagePool(mContext);
    mPackagePool->Initial();

    // Initial Service
    mService->SetUpAgent(this);
    const auto ret = mService->Initial(pData);

    // Context And Service Initialized
    SPDLOG_INFO("{} - Agent[{} - {:p}] Service[{}] Initial Successfully",
        __FUNCTION__, mServiceID, static_cast<const void *>(this), mService->GetServiceName());

    // Delete The Data Asset For Initialization
    delete pData;

    return ret;
}

void UServiceAgent::Stop() {
    if (mChannel.is_open()) {
        mChannel.close();
        SPDLOG_INFO("{} - Agent[{} - {:p}] Will Stop",
            __FUNCTION__, mServiceID, static_cast<const void *>(this));
    }
}

bool UServiceAgent::BootService() {
    if (mModule == nullptr ||
        mPackagePool == nullptr ||
        mService == nullptr ||
        !mChannel.is_open())
        throw std::logic_error(std::format("{:<20} - Not Initialized", __FUNCTION__));

    // Start The Service
    if (const auto res = mService->Start(); !res) {
        SPDLOG_ERROR("{} - Agent[{} - {:p}], Service[{}] Failed To Boot.",
            __FUNCTION__, mServiceID, static_cast<const void *>(this), GetServiceName());

        return false;
    }

    SPDLOG_TRACE("{} - Agent[{} - {:p}], Service[{}] Started.",
        __FUNCTION__, mServiceID,static_cast<const void *>(this), GetServiceName());

    // If It Set Update
    if (mService->bUpdatePerTick) {
        if (auto *module = dynamic_cast<UServiceModule *>(mModule)) {
            module->InsertTicker(mServiceID);
        }
    }

    // Begin The Looping
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
    if (mService == nullptr || !mChannel.is_open())
        return;

    auto node = make_unique<UTickerNode>();
    node->SetCurrentTickTime(timepoint);
    node->SetDeltaTime(delta);

    // Push To The Inner Channel
    if (const bool ret = mChannel.try_send_via_dispatch(std::error_code{}, std::move(node)); !ret && mChannel.is_open()) {
        auto temp = make_unique<UTickerNode>();
        temp->SetCurrentTickTime(timepoint);
        temp->SetDeltaTime(delta);

        co_spawn(mContext, [self = SharedFromThis(), temp = std::move(temp)]() mutable -> awaitable<void> {
            co_await self->mChannel.async_send(std::error_code{}, std::move(temp));
        }, detached);
    }
}

void UServiceAgent::PostPackage(const FPackageHandle &pkg) const {
    if (pkg == nullptr)
        return;

    // Do Not Post Th Self
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

    // Do Not Post Th Self
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

    // Do Not Post Th Self
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

    // Do Not Post Th Self
    if (name.empty() || name == GetServiceName())
        return;

    const auto *router = GetServer()->GetModule<URouteModule>();
    if (!router)
        return;

    router->PostTask(name, task);
}

void UServiceAgent::SendToPlayer(const int64_t pid, const FPackageHandle &pkg) const {
    if (pkg == nullptr || pid < 0)
        return;

    const auto *router = GetServer()->GetModule<URouteModule>();
    if (!router)
        return;

    pkg->SetTarget(PLAYER_TARGET_ID);
    router->PostPackage(pkg, pid);
}

void UServiceAgent::SendToClient(const int64_t pid, const FPackageHandle &pkg) const {
    if (pkg == nullptr || pid <= 0)
        return;

    const auto *router = GetServer()->GetModule<URouteModule>();
    if (!router)
        return;

    pkg->SetTarget(CLIENT_TARGET_ID);
    router->SendToClient(pid, pkg);
}

void UServiceAgent::PostToPlayer(const int64_t pid, const AActorTask &task) const {
    if (task == nullptr || pid <= 0)
        return;

    const auto *router = GetServer()->GetModule<URouteModule>();
    if (!router)
        return;

    router->PostTask(false, pid, task);
}

void UServiceAgent::ListenEvent(const int event) {
    if (mService == nullptr || !mChannel.is_open())
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
    if (mService == nullptr || !mChannel.is_open())
        return;

    auto *module = GetServer()->GetModule<UEventModule>();
    if (module == nullptr)
        return;

    module->Dispatch(param);
}

IActorBase *UServiceAgent::GetActor() const {
    return mService.Get();
}

void UServiceAgent::CleanUp() {
    if (mService == nullptr)
        return;

    mService->Stop();
    SPDLOG_INFO("{} - Agent[{} - {:p}], Service Stopped",
        __FUNCTION__, mServiceID, static_cast<const void *>(this));

    mService.Release();
    mServiceID = INVALID_SERVICE_ID;
}

shared_ptr<UServiceAgent> UServiceAgent::SharedFromThis() {
    return std::dynamic_pointer_cast<UServiceAgent>(shared_from_this());
}

weak_ptr<UServiceAgent> UServiceAgent::WeakFromThis() {
    return this->SharedFromThis();
}
