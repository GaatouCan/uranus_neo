#include "Context.h"
#include "ServiceBase.h"
#include "Server.h"
#include "base/DataAsset.h"
#include "base/Recycler.h"
#include "event/EventModule.h"
#include "timer/TimerModule.h"

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
      mPackagePool(nullptr),
      mChannel(nullptr),
      mShutdownTimer(nullptr) {
}

UContext::~UContext() {
    if (mShutdownTimer) {
        mShutdownTimer->cancel();
        ForceShutdown();
    }

    if (mChannel) {
        mChannel->close();
    }
}


void UContext::SetUpServer(UServer *pServer) {
    if (mState != EContextState::CREATED)
        throw std::logic_error(std::format("{} - Only Can Called In CREATED State", __FUNCTION__));

    mServer = pServer;
}

void UContext::SetUpServiceID(const FServiceHandle sid) {
    if (mState != EContextState::CREATED)
        return;
    mServiceID = sid;
}

void UContext::SetUpLibrary(const FSharedLibrary &library) {
    if (mState != EContextState::CREATED)
        return;
    mLibrary = library;
}

bool UContext::Initial(const IDataAsset_Interface *pData) {
    if (mState != EContextState::CREATED)
        throw std::logic_error(std::format("{} - Only Can Called In CREATED State", __FUNCTION__));

    if (!mLibrary.IsValid()) {
        SPDLOG_ERROR("{:<20} - Owner Module Or Library Node Is Null", __FUNCTION__);
        return false;
    }

    // Start To Create Service
    mState = EContextState::INITIALIZING;

    // Load The Constructor From Shared Library
    auto creator = mLibrary.GetSymbol<AServiceCreator>("CreateInstance");
    if (creator == nullptr) {
        SPDLOG_ERROR("{:<20} - Can't Load Creator", __FUNCTION__);
        mState = EContextState::CREATED;
        return false;
    }

    mService = std::invoke(creator);
    if (mService == nullptr) {
        SPDLOG_ERROR("{:<20} - Can't Create Service", __FUNCTION__);
        mState = EContextState::CREATED;
        return false;
    }

    // Create Node Channel
    mChannel = make_unique<AContextChannel>(mCtx, 1024);

    // Create Package Pool For Data Exchange
    mPackagePool = mServer->CreateUniquePackagePool();
    mPackagePool->Initial();

    // Initial Service
    mService->SetUpContext(this);
    const auto ret = mService->Initial(pData);

    // Context And Service Initialized
    mState = EContextState::INITIALIZED;
    SPDLOG_TRACE("{:<20} - Context[{:p}] Service[{}] Initial Successfully",
        __FUNCTION__, static_cast<const void *>(this), mService->GetServiceName());

    // Delete The Data Asset For Initialization
    delete pData;

    return ret;
}

awaitable<bool> UContext::AsyncInitial(const IDataAsset_Interface *pData) {
    if (mState != EContextState::CREATED)
        throw std::logic_error(std::format("{} - Only Can Called In CREATED State", __FUNCTION__));

    if (!mLibrary.IsValid()) {
        SPDLOG_ERROR("{:<20} - Owner Module Or Library Node Is Null", __FUNCTION__);
        co_return false;
    }

    // Start To Create Service
    mState = EContextState::INITIALIZING;

    auto creator = mLibrary.GetSymbol<AServiceCreator>("CreateInstance");
    if (creator == nullptr) {
        SPDLOG_ERROR("{:<20} - Can't Load Creator", __FUNCTION__);
        mState = EContextState::CREATED;
        co_return false;
    }

    mService = std::invoke(creator);
    if (mService == nullptr) {
        SPDLOG_ERROR("{:<20} - Can't Create Service", __FUNCTION__);
        mState = EContextState::CREATED;
        co_return false;
    }

    // Create Node Channel
    mChannel = make_unique<AContextChannel>(mCtx, 1024);

    // Create Package Pool For Data Exchange
    mPackagePool = mServer->CreateUniquePackagePool();
    mPackagePool->Initial();

    // Initial Service
    mService->SetUpContext(this);
    const auto ret = co_await mService->AsyncInitial(pData);

    // Context And Service Initialized
    mState = EContextState::INITIALIZED;
    SPDLOG_TRACE("{:<20} - Context[{:p}] Service[{}] Initial Successfully",
                 __FUNCTION__, static_cast<const void *>(this), mService->GetServiceName());

    // Delete The Data Asset For Initialization
    delete pData;

    co_return ret;
}

int UContext::Shutdown(const bool bForce, int second, const std::function<void(UContext *)> &func) {
    if (GetServer() == nullptr)
        return -10;

    // State Maybe WAITING While If Not Force To Shut Down
    if (bForce ? mState >= EContextState::SHUTTING_DOWN : mState >= EContextState::WAITING)
        return -1;

    // Do Something While Shutdown
    {
        if (auto *module = GetServer()->GetModule<UTimerModule>()) {
            module->CancelTimer(GenerateHandle());
            module->RemoveTicker(GenerateHandle());
        }

        if (auto *module = GetServer()->GetModule<UEventModule>()) {
            module->RemoveListener(GenerateHandle());
        }
    }

    // If Not Force To Shut Down, Turn To Waiting Current Schedule Node Execute Complete
    if (!bForce && mState == EContextState::RUNNING) {
        mState = EContextState::WAITING;

        mShutdownTimer = make_unique<ASteadyTimer>(GetServer()->GetIOContext());
        if (func != nullptr)
            mShutdownCallback = func;

        second = second <= 0 ? 5 : second;

        // Spawn Coroutine For Waiting To Force Shut Down
        co_spawn(GetServer()->GetIOContext(), [self = shared_from_this(), second]() -> awaitable<void> {
            self->mShutdownTimer->expires_after(std::chrono::seconds(second));
            if (const auto [ec] = co_await self->mShutdownTimer->async_wait(); ec)
                co_return;

            if (self->GetState() == EContextState::WAITING) {
                self->ForceShutdown();
            }
        }, detached);

        return 0;
    }

    mState = EContextState::SHUTTING_DOWN;

    if (mShutdownTimer)
        mShutdownTimer->cancel();

    mChannel->close();

    const std::string name = GetServiceName();

    if (mService && mService->GetState() != EServiceState::TERMINATED) {
        mService->Stop();
    }

    if (!mLibrary.IsValid()) {
        SPDLOG_ERROR("{:<20} - Library Node Is Null", __FUNCTION__);
        mState = EContextState::STOPPED;
        return -2;
    }

    auto destroyer = mLibrary.GetSymbol<AServiceDestroyer>("DestroyInstance");
    if (destroyer == nullptr) {
        SPDLOG_ERROR("{:<20} - Can't Load Destroyer", __FUNCTION__);
        mState = EContextState::STOPPED;
        return -3;
    }

    std::invoke(destroyer, mService);

    mService = nullptr;
    mLibrary.Reset();

    mState = EContextState::STOPPED;
    SPDLOG_TRACE("{:<20} - Context[{:p}] Service[{}] Shut Down Successfully",
                 __FUNCTION__, static_cast<void *>(this), name);

    if (mShutdownCallback) {
        std::invoke(mShutdownCallback, this);
    }

    // Recycle The ServiceID And Make It Invalid
    if (auto *module = GetServer()->GetModule<UServiceModule>()) {
        module->RecycleServiceID(mServiceID);
    }
    mServiceID = INVALID_SERVICE_ID;

    return 1;
}

int UContext::ForceShutdown() {
    return Shutdown(true, 0, nullptr);
}

bool UContext::BootService() {
    if (mState != EContextState::INITIALIZED || mService == nullptr || GetServer() == nullptr)
        return false;

    mState = EContextState::IDLE;

    if (const auto res = mService->Start(); !res) {
        SPDLOG_ERROR("{:<20} - Context[{:p}], Service[{} - {}] Failed To Boot.",
                     __FUNCTION__, static_cast<const void *>(this), static_cast<int64_t>(mServiceID), GetServiceName());

        mState = EContextState::INITIALIZED;
        return false;
    }

    SPDLOG_TRACE("{:<20} - Context[{:p}], Service[{} - {}] Started.",
                 __FUNCTION__, static_cast<const void *>(this), static_cast<int64_t>(mServiceID), GetServiceName());

    co_spawn(GetServer()->GetIOContext(), [self = shared_from_this()] -> awaitable<void> {
        co_await self->ProcessChannel();
    }, detached);

    if (mService->bUpdatePerTick) {
        if (auto *module = GetServer()->GetModule<UTimerModule>()) {
            module->AddTicker(GenerateHandle());
        }
    }

    return true;
}

std::string UContext::GetServiceName() const {
    if (mState >= EContextState::INITIALIZED) {
        return mService->GetServiceName();
    }
    return "UNKNOWN";
}

FServiceHandle UContext::GetServiceID() const {
    return mServiceID;
}


EContextState UContext::GetState() const {
    return mState;
}

void UContext::PushPackage(const FPackageHandle &pkg) {
    if (mState < EContextState::INITIALIZED || mState >= EContextState::WAITING)
        return;

    if (pkg == nullptr)
        return;

    SPDLOG_TRACE("{:<20} - Context[{:p}], Service[{}] - Package From {}",
        __FUNCTION__, static_cast<const void *>(this), GetServiceName(), pkg->GetSource());

    auto node = make_unique<UPackageNode>();
    node->SetPackage(pkg);

    if (const bool ret = mChannel->try_send_via_dispatch(std::error_code{}, std::move(node)); !ret) {
        SPDLOG_WARN("{:<20} - Context[{:p}], Service[{}] Channel Buffer Is Full, Consider Make It Larger",
            __FUNCTION__, static_cast<const void *>(this), GetServiceName());

        auto temp = make_unique<UPackageNode>();
        temp->SetPackage(pkg);

        co_spawn(mCtx, [self = shared_from_this(), temp = std::move(temp)]() mutable -> awaitable<void> {
            co_await self->mChannel->async_send(std::error_code{}, std::move(temp));
        }, detached);
    }
}

void UContext::PushTask(const std::function<void(IServiceBase *)> &task) {
    if (mState < EContextState::INITIALIZED || mState >= EContextState::WAITING)
        return;

    if (task == nullptr)
        return;

    SPDLOG_TRACE("{:<20} - Context[{:p}], Service[{}]",
        __FUNCTION__, static_cast<const void *>(this), GetServiceName());

    auto node = make_unique<UTaskNode>();
    node->SetTask(task);

    if (const bool ret = mChannel->try_send_via_dispatch(std::error_code{}, std::move(node)); !ret) {
        SPDLOG_WARN("{:<20} - Context[{:p}], Service[{}] Channel Buffer Is Full, Consider Make It Larger",
            __FUNCTION__, static_cast<const void *>(this), GetServiceName());

        // Node Has Already Moved
        auto temp = make_unique<UTaskNode>();
        temp->SetTask(task);

        co_spawn(mCtx, [self = shared_from_this(), node = std::move(temp)]() mutable -> awaitable<void> {
            co_await self->mChannel->async_send(std::error_code{}, std::move(node));
        }, detached);
    }
}

void UContext::PushEvent(const shared_ptr<IEventParam_Interface> &event) {
    if (mState < EContextState::INITIALIZED || mState >= EContextState::WAITING)
        return;

    if (event == nullptr)
        return;

    SPDLOG_TRACE("{:<20} - Context[{:p}], Service[{}] - Event Type {}",
        __FUNCTION__, static_cast<const void *>(this), GetServiceName(), event->GetEventType());

    auto node = make_unique<UEventNode>();
    node->SetEventParam(event);

    if (const bool ret = mChannel->try_send_via_dispatch(std::error_code{}, std::move(node)); !ret) {
        SPDLOG_WARN("{:<20} - Context[{:p}], Service[{}] Channel Buffer Is Full, Consider Make It Larger",
            __FUNCTION__, static_cast<const void *>(this), GetServiceName());

        auto temp = make_unique<UEventNode>();
        temp->SetEventParam(event);
        co_spawn(mCtx, [self = shared_from_this(), temp = std::move(temp)]() mutable -> awaitable<void> {
            co_await self->mChannel->async_send(std::error_code{}, std::move(temp));
        }, detached);
    }
}

void UContext::PushTicker(const ASteadyTimePoint timepoint, const ASteadyDuration delta) {
    if (mState < EContextState::INITIALIZED || mState >= EContextState::WAITING)
        return;

    // SPDLOG_TRACE("{:<20} - Context[{:p}], Service[{}]",
    //              __FUNCTION__, static_cast<const void *>(this), GetServiceName());

    auto node = make_unique<UTickerNode>();
    node->SetCurrentTickTime(timepoint);
    node->SetDeltaTime(delta);

    if (const bool ret = mChannel->try_send_via_dispatch(std::error_code{}, std::move(node)); !ret) {
        auto temp = make_unique<UTickerNode>();
        temp->SetCurrentTickTime(timepoint);
        temp->SetDeltaTime(delta);

        co_spawn(mCtx, [self = shared_from_this(), temp = std::move(temp)]() mutable -> awaitable<void> {
            co_await self->mChannel->async_send(std::error_code{}, std::move(temp));
        }, detached);
    }
}

int64_t UContext::CreateTimer(const std::function<void(IServiceBase *)> &task, const int delay, const int rate) {
    if (mState < EContextState::INITIALIZED || GetServer() == nullptr || task == nullptr)
        return 0;

    auto *module = GetServer()->GetModule<UTimerModule>();
    if (module == nullptr)
        return 0;

    return module->CreateTimer(GenerateHandle(), task, delay, rate);
}

void UContext::CancelTimer(const int64_t tid) const {
    if (mState < EContextState::INITIALIZED || GetServer() == nullptr || tid <= 0)
        return;

    auto *module = GetServer()->GetModule<UTimerModule>();
    if (module == nullptr)
        return;

    module->CancelTimer(tid);
}

void UContext::CancelAllTimers() {
    if (mState < EContextState::INITIALIZED || GetServer() == nullptr)
        return;

    if (auto *module = GetServer()->GetModule<UTimerModule>()) {
        module->CancelTimer(GenerateHandle());
    }
}

void UContext::ListenEvent(const int event) {
    if (mState < EContextState::INITIALIZED || GetServer() == nullptr || event <= 0)
        return;

    auto *module = GetServer()->GetModule<UEventModule>();
    if (module == nullptr)
        return;

    module->ListenEvent(GenerateHandle(), event);
}

void UContext::RemoveListener(const int event) {
    if (mState < EContextState::INITIALIZED || GetServer() == nullptr || event <= 0)
        return;

    auto *module = GetServer()->GetModule<UEventModule>();
    if (module == nullptr)
        return;

    module->RemoveListenEvent(GenerateHandle(), event);
}

void UContext::DispatchEvent(const shared_ptr<IEventParam_Interface> &param) const {
    if (mState < EContextState::INITIALIZED || GetServer() == nullptr || param == nullptr)
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
    if (mPackagePool == nullptr)
        throw std::runtime_error(std::format("{} - Package Pool Is Null Pointer", __FUNCTION__));

    return mPackagePool->Acquire<IPackage_Interface>();
}

IServiceBase *UContext::GetOwningService() const {
    if (mState < EContextState::INITIALIZED || mState >= EContextState::WAITING)
        return nullptr;

    return mService;
}

awaitable<void> UContext::ProcessChannel() {
    if (mState <= EContextState::INITIALIZED || mState >= EContextState::WAITING)
        co_return;

    while (mChannel->is_open()) {
        try {
            auto [ec, node] = co_await mChannel->async_receive();
            if (ec)
                co_return;

            if (node == nullptr)
                continue;

            if (mState >= EContextState::WAITING)
                co_return;

            mState = EContextState::RUNNING;
            node->Execute(mService);

            if (mState >= EContextState::WAITING) {
                ForceShutdown();
                co_return;
            }

            mState = EContextState::IDLE;
        } catch (const std::exception &e) {
            SPDLOG_ERROR("{:<20} - {}", __FUNCTION__, e.what());
        }
    }
}
