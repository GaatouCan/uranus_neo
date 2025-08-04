#include "ContextBase.h"
#include "ServiceBase.h"
#include "Server.h"
#include "DataAsset.h"
#include "Network/Network.h"
#include "Base/Package.h"
#include "Base/EventParam.h"

#include <spdlog/spdlog.h>


typedef IServiceBase *(*AServiceCreator)();
typedef void (*AServiceDestroyer)(IServiceBase *);


IContextBase::INodeBase::INodeBase(IServiceBase *service)
    : mService(service) {
}

IServiceBase *IContextBase::INodeBase::GetService() const {
    return mService;
}

void IContextBase::INodeBase::Execute() {
}

IContextBase::UPackageNode::UPackageNode(IServiceBase *service)
    : INodeBase(service) {
}

void IContextBase::UPackageNode::SetPackage(const shared_ptr<IPackage_Interface> &pkg) {
    mPackage = pkg;
}

void IContextBase::UPackageNode::Execute() {
    if (mService != nullptr && mPackage != nullptr) {
        mService->OnPackage(mPackage);
    }
}

IContextBase::UTaskNode::UTaskNode(IServiceBase *service)
    : INodeBase(service) {
}

void IContextBase::UTaskNode::SetTask(const std::function<void(IServiceBase *)> &task) {
    mTask = task;
}

void IContextBase::UTaskNode::Execute() {
    if (mService && mTask) {
        std::invoke(mTask, mService);
    }
}

IContextBase::UEventNode::UEventNode(IServiceBase *service)
    : INodeBase(service) {
}

void IContextBase::UEventNode::SetEventParam(const shared_ptr<IEventParam_Interface> &event) {
    mEvent = event;
}

void IContextBase::UEventNode::Execute() {
    if (mService != nullptr && mEvent != nullptr) {
        mService->OnEvent(mEvent);
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

void IContextBase::SetUpModule(IModuleBase *module) {
    if (mState != EContextState::CREATED)
        return;
    mModule = module;
}

void IContextBase::SetUpLibrary(const FSharedLibrary &library) {
    if (mState != EContextState::CREATED)
        return;
    mLibrary = library;
}

bool IContextBase::Initial(const IDataAsset_Interface *data) {
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
    const auto ret = mService->Initial(data);

    // Context And Service Initialized
    mState = EContextState::INITIALIZED;
    SPDLOG_TRACE("{:<20} - Context[{:p}] Service[{}] Initial Successfully",
        __FUNCTION__, static_cast<const void *>(this), mService->GetServiceName());

    // Delete The Data Asset For Initialization
    delete data;

    return ret;
}

awaitable<bool> IContextBase::AsyncInitial(const IDataAsset_Interface *data) {
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
    const auto ret = co_await mService->AsyncInitial(data);

    // Context And Service Initialized
    mState = EContextState::INITIALIZED;
    SPDLOG_TRACE("{:<20} - Context[{:p}] Service[{}] Initial Successfully",
        __FUNCTION__, static_cast<const void *>(this), mService->GetServiceName());

    // Delete The Data Asset For Initialization
    delete data;

    co_return ret;
}

int IContextBase::Shutdown(const bool bForce, int second, const std::function<void(IContextBase *)> &func) {
    if (GetServer() == nullptr)
        return -10;

    // State Maybe WAITING While If Not Force To Shut Down
    if (bForce ? mState >= EContextState::SHUTTING_DOWN : mState >= EContextState::WAITING)
        return -1;

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

    if (pkg == nullptr)
        return;

    SPDLOG_TRACE("{:<20} - Context[{:p}], Service[{}] - Package From {}",
        __FUNCTION__, static_cast<const void *>(this), GetServiceName(), pkg->GetSource());

    const auto node = make_shared<UPackageNode>(mService);
    node->SetPackage(pkg);

    PushNode(node);
}

void IContextBase::PushTask(const std::function<void(IServiceBase *)> &task) {
    if (mState < EContextState::INITIALIZED || mState >= EContextState::WAITING)
        return;

    if (task == nullptr)
        return;

    SPDLOG_TRACE("{:<20} - Context[{:p}], Service[{}]",
        __FUNCTION__, static_cast<const void *>(this), GetServiceName());

    const auto node = make_shared<UTaskNode>(mService);
    node->SetTask(task);

    PushNode(node);
}

void IContextBase::PushEvent(const shared_ptr<IEventParam_Interface> &event) {
    if (mState < EContextState::INITIALIZED || mState >= EContextState::WAITING)
        return;

    if (event == nullptr)
        return;

    SPDLOG_TRACE("{:<20} - Context[{:p}], Service[{}] - Event Type {}",
        __FUNCTION__, static_cast<const void *>(this), GetServiceName(), event->GetEventType());

    const auto node = make_shared<UEventNode>(mService);
    node->SetEventParam(event);

    PushNode(node);
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

void IContextBase::PushNode(const shared_ptr<INodeBase> &node) {
    if (GetServer() == nullptr)
        return;

    if (mState < EContextState::INITIALIZED || mState >= EContextState::WAITING)
        return;

    if (node == nullptr)
        return;

    co_spawn(GetServer()->GetIOContext(), [self = shared_from_this(), node]() -> awaitable<void> {
        co_await self->mChannel->async_send(std::error_code{}, node);
    }, detached);
}

awaitable<void> IContextBase::ProcessChannel() {
    if (mState <= EContextState::INITIALIZED || mState >= EContextState::WAITING)
        co_return;

    while (mChannel->is_open()) {
        const auto [ec, node] = co_await mChannel->async_receive();
        if (ec)
            co_return;

        if (node == nullptr)
            continue;

        if (mState >= EContextState::WAITING)
            co_return;

        mState = EContextState::RUNNING;
        try {
            node->Execute();
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
