#include "ContextBase.h"
#include "ServiceBase.h"
#include "Module.h"
#include "Server.h"
#include "Base/Recycler.h"
#include "Base/Package.h"
#include "DataAsset.h"

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

void IContextBase::UPackageNode::SetPackage(const shared_ptr<FPackage> &pkg) {
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
    mPackagePool = make_shared<TRecycler<FPackage> >();
    mPackagePool->Initial();

    // Initial Service
    mService->SetUpContext(this);
    mService->Initial(data);

    // Context And Service Initialized
    mState = EContextState::INITIALIZED;
    SPDLOG_TRACE("{:<20} - Context[{:p}] Service[{}] Initial Successfully",
        __FUNCTION__, static_cast<const void *>(this), mService->GetServiceName());

    return true;
}

int IContextBase::Shutdown(bool bForce, int second, const std::function<void(IContextBase *)> &func) {
}

int IContextBase::ForceShutdown() {
    return Shutdown(true, 0, nullptr);
}

bool IContextBase::BootService() {
}

UServer *IContextBase::GetServer() const {
    if (mModule == nullptr)
        return nullptr;
    return mModule->GetServer();
}

IModuleBase *IContextBase::GetOwnerModule() const {
    return mModule;
}

shared_ptr<FPackage> IContextBase::BuildPackage() const {
    if (mState != EContextState::IDLE || mState != EContextState::RUNNING)
        return nullptr;

    if (const auto elem = mPackagePool->Acquire())
        return std::dynamic_pointer_cast<FPackage>(elem);

    return nullptr;
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
