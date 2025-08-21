#include "Context.h"
#include "ServiceBase.h"
#include "Server.h"
#include "base/Package.h"
#include "base/DataAsset.h"

// #include "event/EventModule.h"
// #include "timer/TimerModule.h"

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
      mPool(nullptr) {
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
    mPool = mServer->CreateUniquePackagePool();
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

    // if (mService->bUpdatePerTick) {
    //     if (auto *module = GetServer()->GetModule<UTimerModule>()) {
    //         module->AddTicker(GenerateHandle());
    //     }
    // }

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

void UContext::PushTask(const std::function<void(IServiceBase *)> &task) {
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

// int64_t UContext::CreateTimer(const std::function<void(IServiceBase *)> &task, const int delay, const int rate) {
//     if (mService == nullptr || mPackagePool == nullptr)
//         return 0;
//
//     auto *module = GetServer()->GetModule<UTimerModule>();
//     if (module == nullptr)
//         return 0;
//
//     return module->CreateTimer(GenerateHandle(), task, delay, rate);
// }
//
// void UContext::CancelTimer(const int64_t tid) const {
//     auto *module = GetServer()->GetModule<UTimerModule>();
//     if (module == nullptr)
//         return;
//
//     module->CancelTimer(tid);
// }
//
// void UContext::CancelAllTimers() {
//     if (auto *module = GetServer()->GetModule<UTimerModule>()) {
//         module->CancelTimer(GenerateHandle());
//     }
// }
//
// void UContext::ListenEvent(const int event) {
//     if (mService == nullptr || mPackagePool == nullptr)
//         return;
//
//     auto *module = GetServer()->GetModule<UEventModule>();
//     if (module == nullptr)
//         return;
//
//     module->ListenEvent(GenerateHandle(), event);
// }
//
// void UContext::RemoveListener(const int event) {
//     auto *module = GetServer()->GetModule<UEventModule>();
//     if (module == nullptr)
//         return;
//
//     module->RemoveListenEvent(GenerateHandle(), event);
// }
//
// void UContext::DispatchEvent(const shared_ptr<IEventParam_Interface> &param) const {
//     if (mService == nullptr || mPackagePool == nullptr)
//         return;
//
//     auto *module = GetServer()->GetModule<UEventModule>();
//     if (module == nullptr)
//         return;
//
//     module->Dispatch(param);
// }

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

// IServiceBase *UContext::GetOwningService() const {
//     if (mService == nullptr)
//         throw std::runtime_error(std::format("{} - Service Is Null Pointer", __FUNCTION__));
//     return mService;
// }

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

    mServiceID = INVALID_SERVICE_ID;
}
