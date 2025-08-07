#include "ContextBase.h"
#include "ServiceBase.h"
#include "Server.h"
#include "DataAsset.h"
#include "Network/Network.h"
#include "Base/Package.h"
#include "Base/EventParam.h"

#include <spdlog/spdlog.h>

#include "Timer/TimerModule.h"


typedef IServiceBase *(*AServiceCreator)();
typedef void (*AServiceDestroyer)(IServiceBase *);


void IContextBase::UPackageNode::SetPackage(const shared_ptr<IPackage_Interface> &pkg) {
    mPackage = pkg;
}

void IContextBase::UPackageNode::Execute(IServiceBase *pService) {
    if (pService && mPackage ) {
        pService->OnPackage(mPackage);
    }
}

void IContextBase::UTaskNode::SetTask(const std::function<void(IServiceBase *)> &task) {
    mTask = task;
}

void IContextBase::UTaskNode::Execute(IServiceBase *pService) {
    if (pService && mTask) {
        std::invoke(mTask, pService);
    }
}

void IContextBase::UEventNode::SetEventParam(const shared_ptr<IEventParam_Interface> &event) {
    mEvent = event;
}

void IContextBase::UEventNode::Execute(IServiceBase *pService) {
    if (pService && mEvent ) {
        pService->OnEvent(mEvent);
    }
}

IContextBase::UTickerNode::UTickerNode()
    : mDeltaTime(0) {
}

void IContextBase::UTickerNode::SetCurrentTickTime(const ASteadyTimePoint timepoint) {
    mTickTime = timepoint;
}

void IContextBase::UTickerNode::SetDeltaTime(const ASteadyDuration delta) {
    mDeltaTime = delta;
}

void IContextBase::UTickerNode::Execute(IServiceBase *pService) {
    if (pService) {
        pService->OnUpdate(mTickTime, mDeltaTime);
    }
}

IContextBase::IContextBase()
    : mModule(nullptr),
      mService(nullptr) {
}

IContextBase::~IContextBase() {
    if (mShutdownTimer) {
        mShutdownTimer->cancel();
        ForceShutdown();
    }

    if (mChannel) {
        mChannel->close();
    }
}

void IContextBase::SetUpModule(IModuleBase *pModule) {
    if (mState != EContextState::CREATED)
        return;
    mModule = pModule;
}

void IContextBase::SetUpLibrary(const FSharedLibrary &library) {
    if (mState != EContextState::CREATED)
        return;
    mLibrary = library;
}

bool IContextBase::Initial(const IDataAsset_Interface *pData) {
    if (mState != EContextState::CREATED)
        return false;

    if (mModule == nullptr || !mLibrary.IsValid()) {
        SPDLOG_ERROR("{:<20} - Owner Module Or Library Node Is Null", __FUNCTION__);
        return false;
    }

    if (GetServer() == nullptr)
        return false;

    const auto network = GetServer()->GetModule<UNetwork>();
    if (network == nullptr) {
        SPDLOG_CRITICAL("Network Module Not Found");
        GetServer()->Shutdown();
        exit(-1);
    }

    // Start To Create Service
    mState = EContextState::INITIALIZING;

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
    mChannel = make_unique<AContextChannel>(GetServer()->GetIOContext(), 1024);

    // Create Package Pool For Data Exchange
    mPackagePool = network->CreatePackagePool();
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

awaitable<bool> IContextBase::AsyncInitial(const IDataAsset_Interface *pData) {
    if (mState != EContextState::CREATED)
        co_return false;

    if (mModule == nullptr || !mLibrary.IsValid()) {
        SPDLOG_ERROR("{:<20} - Owner Module Or Library Node Is Null", __FUNCTION__);
        co_return false;
    }

    if (GetServer() == nullptr)
        co_return false;

    const auto network = GetServer()->GetModule<UNetwork>();
    if (network == nullptr) {
        SPDLOG_CRITICAL("Network Module Not Found");
        GetServer()->Shutdown();
        exit(-1);
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
    mChannel = make_unique<AContextChannel>(GetServer()->GetIOContext(), 1024);

    // Create Package Pool For Data Exchange
    mPackagePool = network->CreatePackagePool();
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

int IContextBase::Shutdown(const bool bForce, int second, const std::function<void(IContextBase *)> &func) {
    if (GetServer() == nullptr)
        return -10;

    // State Maybe WAITING While If Not Force To Shut Down
    if (bForce ? mState >= EContextState::SHUTTING_DOWN : mState >= EContextState::WAITING)
        return -1;

    // Do Something While Shutdown
    {
        if (auto *module = GetServer()->GetModule<UTimerModule>()) {
            module->CancelTimer(weak_from_this());
        }
    }

    // If Not Force To Shut Down, Turn To Waiting Current Schedule Node Execute Complete
    if (!bForce && mState == EContextState::RUNNING) {
        mState = EContextState::WAITING;

        mShutdownTimer = make_unique<ASteadyTimer>(GetServer()->GetIOContext());
        if (func != nullptr)
            mShutdownCallback = func;

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
        // SPDLOG_ERROR("{:<20} - Can't Load Destroyer, Path[{}]", __FUNCTION__, mHandle->GetPath());
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

    return 1;
}

int IContextBase::ForceShutdown() {
    return Shutdown(true, 0, nullptr);
}

bool IContextBase::BootService() {
    if (mState != EContextState::INITIALIZED || mService == nullptr)
        return false;

    mState = EContextState::IDLE;

    if (const auto res = mService->Start(); !res) {
        SPDLOG_ERROR("{:<20} - Context[{:p}], Service[{} - {}] Failed To Boot.",
            __FUNCTION__, static_cast<const void *>(this), GetServiceID(), GetServiceName());

        mState = EContextState::INITIALIZED;
        return false;
    }

    SPDLOG_TRACE("{:<20} - Context[{:p}], Service[{} - {}] Started.",
        __FUNCTION__, static_cast<const void *>(this), GetServiceID(), GetServiceName());

    co_spawn(GetServer()->GetIOContext(), [self = shared_from_this()] -> awaitable<void> {
        co_await self->ProcessChannel();
    }, detached);

    if (mService->bUpdatePerTick) {
        if (auto *module = GetServer()->GetModule<UTimerModule>()) {
            module->AddTicker(weak_from_this());
        }
    }

    return true;
}

std::string IContextBase::GetServiceName() const {
    if (mState >= EContextState::INITIALIZED) {
        return mService->GetServiceName();
    }
    return "UNKNOWN";
}


EContextState IContextBase::GetState() const {
    return mState;
}

void IContextBase::PushPackage(const shared_ptr<IPackage_Interface> &pkg) {
    if (mState < EContextState::INITIALIZED || mState >= EContextState::WAITING)
        return;

    if (GetServer() == nullptr || pkg == nullptr)
        return;

    SPDLOG_TRACE("{:<20} - Context[{:p}], Service[{}] - Package From {}",
        __FUNCTION__, static_cast<const void *>(this), GetServiceName(), pkg->GetSource());

    auto node = make_unique<UPackageNode>();
    node->SetPackage(pkg);

    co_spawn(GetServer()->GetIOContext(), [self = shared_from_this(), node = std::move(node)]() mutable -> awaitable<void> {
        co_await self->mChannel->async_send(std::error_code{}, std::move(node));
    }, detached);
}

void IContextBase::PushTask(const std::function<void(IServiceBase *)> &task) {
    if (mState < EContextState::INITIALIZED || mState >= EContextState::WAITING)
        return;

    if (GetServer() == nullptr || task == nullptr)
        return;

    SPDLOG_TRACE("{:<20} - Context[{:p}], Service[{}]",
        __FUNCTION__, static_cast<const void *>(this), GetServiceName());

    auto node = make_unique<UTaskNode>();
    node->SetTask(task);

    co_spawn(GetServer()->GetIOContext(), [self = shared_from_this(), node = std::move(node)]() mutable -> awaitable<void> {
        co_await self->mChannel->async_send(std::error_code{}, std::move(node));
    }, detached);
}

void IContextBase::PushEvent(const shared_ptr<IEventParam_Interface> &event) {
    if (mState < EContextState::INITIALIZED || mState >= EContextState::WAITING)
        return;

    if (GetServer() == nullptr || event == nullptr)
        return;

    SPDLOG_TRACE("{:<20} - Context[{:p}], Service[{}] - Event Type {}",
        __FUNCTION__, static_cast<const void *>(this), GetServiceName(), event->GetEventType());

    auto node = make_unique<UEventNode>();
    node->SetEventParam(event);

    co_spawn(GetServer()->GetIOContext(), [self = shared_from_this(), node = std::move(node)]() mutable -> awaitable<void> {
        co_await self->mChannel->async_send(std::error_code{}, std::move(node));
    }, detached);
}

void IContextBase::PushTicker(const ASteadyTimePoint timepoint, const ASteadyDuration delta) {
    if (mState < EContextState::INITIALIZED || mState >= EContextState::WAITING)
        return;

    if (GetServer() == nullptr)
        return;

    SPDLOG_TRACE("{:<20} - Context[{:p}], Service[{}]",
        __FUNCTION__, static_cast<const void *>(this), GetServiceName());

    auto node = make_unique<UTickerNode>();
    node->SetCurrentTickTime(timepoint);
    node->SetDeltaTime(delta);

    co_spawn(GetServer()->GetIOContext(), [self = shared_from_this(), node = std::move(node)]() mutable -> awaitable<void> {
        co_await self->mChannel->async_send(std::error_code{}, std::move(node));
    }, detached);
}

int64_t IContextBase::CreateTimer(const std::function<void(IServiceBase *)> &task, const int delay, const int rate) {
    if (mState < EContextState::INITIALIZED || GetServer() == nullptr || task == nullptr)
        return 0;

    auto *module = GetServer()->GetModule<UTimerModule>();
    if (module == nullptr)
        return 0;

    return module->CreateTimer(weak_from_this(), task, delay, rate);
}

void IContextBase::CancelTimer(const int64_t tid) const {
    if (mState < EContextState::INITIALIZED || GetServer() == nullptr || tid <= 0)
        return;

    auto *module = GetServer()->GetModule<UTimerModule>();
    if (module == nullptr)
        return;

    module->CancelTimer(tid);
}

UServer *IContextBase::GetServer() const {
    if (mModule == nullptr)
        return nullptr;
    return mModule->GetServer();
}

IModuleBase *IContextBase::GetOwnerModule() const {
    return mModule;
}

shared_ptr<IPackage_Interface> IContextBase::BuildPackage() const {
    if (mState != EContextState::IDLE || mState != EContextState::RUNNING)
        return nullptr;

    if (const auto elem = mPackagePool->Acquire())
        return std::dynamic_pointer_cast<IPackage_Interface>(elem);

    return nullptr;
}

IServiceBase *IContextBase::GetOwningService() const {
    if (mState < EContextState::INITIALIZED || mState >= EContextState::WAITING)
        return nullptr;

    return mService;
}

awaitable<void> IContextBase::ProcessChannel() {
    if (mState <= EContextState::INITIALIZED || mState >= EContextState::WAITING)
        co_return;

    while (mChannel->is_open()) {
        auto [ec, node] = co_await mChannel->async_receive();
        if (ec)
            co_return;

        if (node == nullptr)
            continue;

        if (mState >= EContextState::WAITING)
            co_return;

        mState = EContextState::RUNNING;
        try {
            node->Execute(mService);
        } catch (const std::exception &e) {
            SPDLOG_ERROR("{:<20} - {}", __FUNCTION__, e.what());
        }

        if (mState >= EContextState::WAITING) {
            ForceShutdown();
            co_return;
        }

        mState = EContextState::IDLE;
    }
}
