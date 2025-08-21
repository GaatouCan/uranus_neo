#include "Context.h"
#include "ServiceBase.h"
#include "Server.h"
#include "Agent.h"
#include "base/Package.h"
#include "base/DataAsset.h"
#include "event/EventModule.h"
#include "timer/TickerModule.h"

#include <spdlog/spdlog.h>


typedef IServiceBase *(*AServiceCreator)();
typedef void (*AServiceDestroyer)(IServiceBase *);

using std::make_unique;


#pragma region Schedule
void UContext::UPackageNode::SetPackage(const FPackageHandle &pkg) {
    mPackage = pkg;
}

void UContext::UPackageNode::Execute(IServiceBase *pService) {
    if (pService && mPackage.IsValid()) {
        pService->OnPackage(mPackage.Get());
    }
}

void UContext::UTaskNode::SetTask(const std::function<void(IServiceBase *)> &task) {
    mTask = task;
}

void UContext::UTaskNode::Execute(IServiceBase *pService) {
    if (pService && mTask) {
        std::invoke(mTask, pService);
    }
}

void UContext::UEventNode::SetEventParam(const shared_ptr<IEventParam_Interface> &event) {
    mEvent = event;
}

void UContext::UEventNode::Execute(IServiceBase *pService) {
    if (pService && mEvent) {
        pService->OnEvent(mEvent.get());
    }
}

UContext::UTickerNode::UTickerNode()
    : mDeltaTime(0) {
}

void UContext::UTickerNode::SetCurrentTickTime(const ASteadyTimePoint timepoint) {
    mTickTime = timepoint;
}

void UContext::UTickerNode::SetDeltaTime(const ASteadyDuration delta) {
    mDeltaTime = delta;
}

void UContext::UTickerNode::Execute(IServiceBase *pService) {
    if (pService) {
        pService->OnUpdate(mTickTime, mDeltaTime);
    }
}
#pragma endregion

UContext::UContext(asio::io_context &ctx)
    : mCtx(ctx),
      mServer(nullptr),
      mServiceID(INVALID_SERVICE_ID),
      mService(nullptr),
      mChannel(mCtx, 1024),
      mPool(nullptr),
      mTimerManager(mCtx) {
}

UContext::~UContext() {
    Stop();
}


void UContext::SetUpServer(UServer *pServer) {
    mServer = pServer;
}

void UContext::SetUpServiceID(const FServiceHandle sid) {
    mServiceID = sid;
}

void UContext::SetUpLibrary(const FSharedLibrary &library) {
    mLibrary = library;
}

bool UContext::Initial(const IDataAsset_Interface *pData) {
    if (mServer == nullptr || !mLibrary.IsValid())
        throw std::logic_error(std::format("{} - Server Is Null", __FUNCTION__));

    if (mService != nullptr)
        return false;

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

    // Create Package Pool For Data Exchange
    mPool = mServer->CreateUniquePackagePool(mCtx);
    mPool->Initial();

    // Initial Service
    mService->SetUpContext(this);
    const auto ret = mService->Initial(pData);

    // Context And Service Initialized
    SPDLOG_TRACE("{:<20} - Context[{:p}] Service[{}] Initial Successfully",
        __FUNCTION__, static_cast<const void *>(this), mService->GetServiceName());

    // Delete The Data Asset For Initialization
    delete pData;

    return ret;
}

void UContext::Stop() {
    if (!mChannel.is_open())
        return;

    mChannel.close();
}

bool UContext::BootService() {
    if (mServer == nullptr || !mLibrary.IsValid() || mService == nullptr || mPool == nullptr)
        throw std::logic_error(std::format("{:<20} - Not Initialized", __FUNCTION__));

    if (const auto res = mService->Start(); !res) {
        SPDLOG_ERROR("{:<20} - Context[{:p}], Service[{} - {}] Failed To Boot.",
            __FUNCTION__, static_cast<const void *>(this), static_cast<int64_t>(mServiceID), GetServiceName());

        return false;
    }

    SPDLOG_TRACE("{:<20} - Context[{:p}], Service[{} - {}] Started.",
        __FUNCTION__, static_cast<const void *>(this), static_cast<int64_t>(mServiceID), GetServiceName());

    if (mService->bUpdatePerTick) {
        if (auto *module = GetServer()->GetModule<UTickerModule>()) {
            module->AddTicker(GenerateHandle());
        }
    }

    co_spawn(mCtx, [self = shared_from_this()] -> awaitable<void> {
        co_await self->ProcessChannel();
    }, detached);

    return true;
}

std::string UContext::GetServiceName() const {
    if (mService)
        return mService->GetServiceName();
    return "UNKNOWN";
}

FServiceHandle UContext::GetServiceID() const {
    return mServiceID;
}

void UContext::PushPackage(const FPackageHandle &pkg) {
    if (mService == nullptr || !mChannel.is_open())
        return;

    if (pkg == nullptr)
        return;

    SPDLOG_TRACE("{:<20} - Context[{:p}], Service[{}] - Package From {}",
        __FUNCTION__, static_cast<const void *>(this), GetServiceName(), pkg->GetSource());

    auto node = make_unique<UPackageNode>();
    node->SetPackage(pkg);

    if (const bool ret = mChannel.try_send_via_dispatch(std::error_code{}, std::move(node)); !ret && mChannel.is_open()) {
        SPDLOG_WARN("{:<20} - Context[{:p}], Service[{}] Channel Buffer Is Full, Consider Make It Larger",
            __FUNCTION__, static_cast<const void *>(this), GetServiceName());

        auto temp = make_unique<UPackageNode>();
        temp->SetPackage(pkg);

        co_spawn(mCtx, [self = shared_from_this(), temp = std::move(temp)]() mutable -> awaitable<void> {
            co_await self->mChannel.async_send(std::error_code{}, std::move(temp));
        }, detached);
    }
}

void UContext::PushTask(const AServiceTask &task) {
    if (mService == nullptr || !mChannel.is_open())
        return;

    if (task == nullptr)
        return;

    SPDLOG_TRACE("{:<20} - Context[{:p}], Service[{}]",
        __FUNCTION__, static_cast<const void *>(this), GetServiceName());

    auto node = make_unique<UTaskNode>();
    node->SetTask(task);

    if (const bool ret = mChannel.try_send_via_dispatch(std::error_code{}, std::move(node)); !ret && mChannel.is_open()) {
        SPDLOG_WARN("{:<20} - Context[{:p}], Service[{}] Channel Buffer Is Full, Consider Make It Larger",
            __FUNCTION__, static_cast<const void *>(this), GetServiceName());

        // Node Has Already Moved
        auto temp = make_unique<UTaskNode>();
        temp->SetTask(task);

        co_spawn(mCtx, [self = shared_from_this(), node = std::move(temp)]() mutable -> awaitable<void> {
            co_await self->mChannel.async_send(std::error_code{}, std::move(node));
        }, detached);
    }
}

void UContext::PushEvent(const shared_ptr<IEventParam_Interface> &event) {
    if (mService == nullptr || !mChannel.is_open())
        return;

    if (event == nullptr)
        return;

    SPDLOG_TRACE("{:<20} - Context[{:p}], Service[{}] - Event Type {}",
        __FUNCTION__, static_cast<const void *>(this), GetServiceName(), event->GetEventType());

    auto node = make_unique<UEventNode>();
    node->SetEventParam(event);

    if (const bool ret = mChannel.try_send_via_dispatch(std::error_code{}, std::move(node)); !ret && mChannel.is_open()) {
        SPDLOG_WARN("{:<20} - Context[{:p}], Service[{}] Channel Buffer Is Full, Consider Make It Larger",
            __FUNCTION__, static_cast<const void *>(this), GetServiceName());

        auto temp = make_unique<UEventNode>();
        temp->SetEventParam(event);
        co_spawn(mCtx, [self = shared_from_this(), temp = std::move(temp)]() mutable -> awaitable<void> {
            co_await self->mChannel.async_send(std::error_code{}, std::move(temp));
        }, detached);
    }
}

void UContext::PushTicker(const ASteadyTimePoint timepoint, const ASteadyDuration delta) {
    if (mService == nullptr || !mChannel.is_open())
        return;

    // SPDLOG_TRACE("{:<20} - Context[{:p}], Service[{}]",
    //              __FUNCTION__, static_cast<const void *>(this), GetServiceName());

    auto node = make_unique<UTickerNode>();
    node->SetCurrentTickTime(timepoint);
    node->SetDeltaTime(delta);

    if (const bool ret = mChannel.try_send_via_dispatch(std::error_code{}, std::move(node)); !ret && mChannel.is_open()) {
        auto temp = make_unique<UTickerNode>();
        temp->SetCurrentTickTime(timepoint);
        temp->SetDeltaTime(delta);

        co_spawn(mCtx, [self = shared_from_this(), temp = std::move(temp)]() mutable -> awaitable<void> {
            co_await self->mChannel.async_send(std::error_code{}, std::move(temp));
        }, detached);
    }
}

void UContext::PostPackage(const FPackageHandle &pkg) const {
    if (pkg == nullptr)
        return;

    const int32_t target = pkg->GetTarget();
    if (target < 0 || target == mServiceID)
        return;

    if (const auto context = GetServer()->FindService(target)) {
        SPDLOG_TRACE("{:<20} - From Service[ID: {}, Name: {}] To Service[ID: {}, Name: {}]",
            __FUNCTION__, static_cast<int>(mServiceID), GetServiceName(), target, context->GetServiceName());

        pkg->SetSource(mServiceID);
        context->PushPackage(pkg);
    }
}

void UContext::PostPackage(const std::string &name, const FPackageHandle &pkg) const {
    if (mService == nullptr)
        return;

    if (name.empty())
        return;

    const auto selfName = GetServiceName();
    if (selfName.empty() || selfName == "UNKNOWN")
        return;

    if (selfName == name)
        return;

    if (const auto target = GetServer()->FindService(name)) {
        const auto targetID = target->GetServiceID();

        SPDLOG_TRACE("{:<20} - From Service[ID: {}, Name: {}] To Service[ID: {}, Name: {}]",
            __FUNCTION__, static_cast<int>(mServiceID), selfName, static_cast<int>(targetID), target->GetServiceName());

        pkg->SetSource(mServiceID);
        pkg->SetTarget(targetID);

        target->PushPackage(pkg);
    }
}

void UContext::PostTask(const int64_t target, const AServiceTask &task) const {
    if (task == nullptr)
        return;

    if (target < 0 || target == static_cast<int64_t>(mServiceID))
        return;

    if (const auto context = GetServer()->FindService(target)) {
        SPDLOG_TRACE("{:<20} - From Service[ID: {}, Name: {}] To Service[ID: {}, Name: {}]",
            __FUNCTION__, static_cast<int>(mServiceID), GetServiceName(), target, context->GetServiceName());

        context->PushTask(task);
    }
}

void UContext::PostTask(const std::string &name, const AServiceTask &task) const {
    if (mService == nullptr)
        return;

    const auto selfName = GetServiceName();
    if (selfName.empty() || selfName == "UNKNOWN")
        return;

    if (selfName == name)
        return;

    if (const auto target = GetServer()->FindService(name)) {
        SPDLOG_TRACE("{:<20} - From Service[ID: {}, Name: {}] To Service[ID: {}, Name: {}]",
            __FUNCTION__, static_cast<int>(mServiceID), GetServiceName(), static_cast<int>(target->GetServiceID()), target->GetServiceName());

        target->PushTask(task);
    }
}

void UContext::SendToPlayer(const int64_t pid, const FPackageHandle &pkg) const {
    if (pid <= 0 || pkg == nullptr)
        return;

    const auto agent = GetServer()->FindPlayer(pid);
    if (agent == nullptr)
        return;

    SPDLOG_TRACE("{:<20} - From Service[ID: {}, Name: {}] To Player[{}]",
        __FUNCTION__, static_cast<int>(mServiceID), GetServiceName(), pid);

    pkg->SetSource(mServiceID);
    pkg->SetTarget(PLAYER_TARGET_ID);

    agent->PushPackage(pkg);
}

void UContext::SendToClient(const int64_t pid, const FPackageHandle &pkg) const {
    if (pid <= 0 || pkg == nullptr)
        return;

    const auto agent = GetServer()->FindPlayer(pid);
    if (agent == nullptr)
        return;

    SPDLOG_TRACE("{:<20} - From Service[ID: {}, Name: {}] To Player[{}]",
        __FUNCTION__, static_cast<int>(mServiceID), GetServiceName(), pid);

    pkg->SetSource(mServiceID);
    pkg->SetTarget(CLIENT_TARGET_ID);

    agent->SendPackage(pkg);
}

void UContext::PostToPlayer(const int64_t pid, const APlayerTask &task) const {
    if (pid <= 0 || task == nullptr)
        return;

    const auto agent = GetServer()->FindPlayer(pid);
    if (agent == nullptr)
        return;

    agent->PushTask(task);
}

FTimerHandle UContext::CreateTimer(const ATimerTask &task, const int delay, const int rate) {
    if (mService == nullptr || !mChannel.is_open())
        return {};

    return mTimerManager.CreateTimer(task, delay, rate);
}

void UContext::CancelTimer(const int64_t tid) const {
    mTimerManager.CancelTimer(tid);
}

void UContext::CancelAllTimers() {
    mTimerManager.CancelAll();
}

void UContext::ListenEvent(const int event) {
    if (mService == nullptr || !mChannel.is_open())
        return;

    auto *module = GetServer()->GetModule<UEventModule>();
    if (module == nullptr)
        return;

    module->ServiceListenEvent(GenerateHandle(), event);
}

void UContext::RemoveListener(const int event) {
    auto *module = GetServer()->GetModule<UEventModule>();
    if (module == nullptr)
        return;

    module->RemoveServiceListenerByEvent(GenerateHandle(), event);
}

void UContext::DispatchEvent(const shared_ptr<IEventParam_Interface> &param) const {
    if (mService == nullptr || !mChannel.is_open())
        return;

    auto *module = GetServer()->GetModule<UEventModule>();
    if (module == nullptr)
        return;

    module->Dispatch(param);
}

UServer *UContext::GetServer() const {
    if (mServer == nullptr)
        throw std::runtime_error(std::format("{} - Server Is Null Pointer", __FUNCTION__));
    return mServer;
}

asio::io_context &UContext::GetIOContext() const {
    return mCtx;
}


FContextHandle UContext::GenerateHandle() {
    if (static_cast<int64_t>(mServiceID) == INVALID_SERVICE_ID)
        return {};

    return { mServiceID, weak_from_this() };
}

FPackageHandle UContext::BuildPackage() const {
    if (mPool == nullptr)
        throw std::runtime_error(std::format("{} - Package Pool Is Null Pointer", __FUNCTION__));

    return mPool->Acquire<IPackage_Interface>();
}

awaitable<void> UContext::ProcessChannel() {
    try {
        while (mChannel.is_open()) {
            auto [ec, node] = co_await mChannel.async_receive();
            if (ec || !mChannel.is_open())
                break;

            if (node == nullptr)
                continue;

            node->Execute(mService);
        }
    } catch (const std::exception &e) {
        SPDLOG_ERROR("{} - {}", __FUNCTION__, e.what());
    }

    CleanUp();
}

void UContext::CleanUp() {
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

    if (auto *module = GetServer()->GetModule<UEventModule>()) {
        module->RemoveServiceListener(mServiceID);
    }

    if (auto *module = GetServer()->GetModule<UTickerModule>()) {
        module->RemoveTicker(mServiceID);
    }

    mServiceID = INVALID_SERVICE_ID;
}
