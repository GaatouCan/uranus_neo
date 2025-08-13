#include "ServiceBase.h"
#include "ContextBase.h"
#include "base/Package.h"
#include "service/ServiceModule.h"
#include "service/ServiceContext.h"
#include "gateway/Gateway.h"
#include "logger/LoggerModule.h"

#include <spdlog/spdlog.h>


void IServiceBase::SetUpContext(UContextBase *pContext) {
    if (mState != EServiceState::CREATED)
        return;
    mContext = pContext;
}

UContextBase *IServiceBase::GetContext() const {
    return mContext;
}

IServiceBase::IServiceBase()
    : mContext(nullptr),
      mState(EServiceState::CREATED),
      bUpdatePerTick(true) {
}

IServiceBase::~IServiceBase() {
}

std::string IServiceBase::GetServiceName() const {
    return {};
}

int32_t IServiceBase::GetServiceID() const {
    if (mContext == nullptr)
        return INVALID_SERVICE_ID;
    return mContext->GetServiceID();
}

io_context &IServiceBase::GetIOContext() const {
    return mContext->GetServer()->GetIOContext();
}

UServer *IServiceBase::GetServer() const {
    if (mContext == nullptr)
        return nullptr;
    return mContext->GetServer();
}

shared_ptr<IPackage_Interface> IServiceBase::BuildPackage() const {
    if (mState != EServiceState::RUNNING)
        return nullptr;

    if (mContext == nullptr)
        return nullptr;

    if (auto pkg = mContext->BuildPackage()) {
        pkg->SetSource(GetServiceID());
        return pkg;
    }

    return nullptr;
}

void IServiceBase::PostPackage(const shared_ptr<IPackage_Interface> &pkg) const {
    if (mState != EServiceState::RUNNING)
        return;

    if (pkg == nullptr)
        return;

    // Do Not Post To Self
    const auto target = pkg->GetTarget();
    if (target <= 0 || target == GetServiceID())
        return;

    const auto *module = GetModule<UServiceModule>();
    if (module == nullptr)
        return;

    if (const auto context = module->FindService(target)) {
        SPDLOG_TRACE("{:<20} - From Service[ID: {}, Name: {}] To Service[ID: {}, Name: {}]",
        __FUNCTION__, GetServiceID(), GetServiceName(), target, context->GetServiceName());

        pkg->SetSource(GetServiceID());
        context->PushPackage(pkg);
    }
}

void IServiceBase::PostPackage(const std::string &name, const shared_ptr<IPackage_Interface> &pkg) const {
    if (mState != EServiceState::RUNNING)
        return;

    // Do Not Post To Self
    if (pkg == nullptr || name == GetServiceName())
        return;

    const auto *module = GetModule<UServiceModule>();
    if (module == nullptr)
        return;

    if (const auto target = module->FindService(name)) {
        SPDLOG_TRACE("{:<20} - From Service[ID: {}, Name: {}] To Service[ID: {}, Name: {}]",
            __FUNCTION__, GetServiceID(), GetServiceName(), static_cast<int>(target->GetServiceID()), target->GetServiceName());

        pkg->SetSource(GetServiceID());
        pkg->SetTarget(target->GetServiceID());

        target->PushPackage(pkg);
    }
}

void IServiceBase::PostTask(int32_t target, const std::function<void(IServiceBase *)> &task) const {
    if (mState != EServiceState::RUNNING)
        return;

    // Do Not Post To Self
    if (target < 0 || target == GetServiceID())
        return;

    const auto *module = GetModule<UServiceModule>();
    if (module == nullptr)
        return;

    if (const auto context = module->FindService(target)) {
        SPDLOG_TRACE("{:<20} - From Service[ID: {}, Name: {}] To Service[ID: {}, Name: {}]",
            __FUNCTION__, GetServiceID(), GetServiceName(), target, context->GetServiceName());

        context->PushTask(task);
    }
}

void IServiceBase::PostTask(const std::string &name, const std::function<void(IServiceBase *)> &task) const {
    if (mState != EServiceState::RUNNING)
        return;

    // Do Not Post To Self
    if (name == GetServiceName())
        return;

    const auto *module = GetModule<UServiceModule>();
    if (module == nullptr)
        return;

    if (const auto target = module->FindService(name)) {
        SPDLOG_TRACE("{:<20} - From Service[ID: {}, Name: {}] To Service[ID: {}, Name: {}]",
            __FUNCTION__, GetServiceID(), GetServiceName(), static_cast<int>(target->GetServiceID()), target->GetServiceName());

        target->PushTask(task);
    }
}

void IServiceBase::SendToPlayer(int64_t pid, const shared_ptr<IPackage_Interface> &pkg) const {
    if (mState != EServiceState::RUNNING)
        return;

    if (pkg == nullptr)
        return;

    if (const auto *gateway = GetModule<UGateway>()) {
        SPDLOG_TRACE("{:<20} - From Service[ID: {}, Name: {}] To Player[{}]",
        __FUNCTION__, GetServiceID(), GetServiceName(), pid);

        pkg->SetSource(GetServiceID());
        pkg->SetTarget(PLAYER_AGENT_ID);

        gateway->SendToPlayer(pid, pkg);
    }
}

void IServiceBase::PostToPlayer(int64_t pid, const std::function<void(IServiceBase *)> &task) const {
    if (mState != EServiceState::RUNNING)
        return;

    if (task == nullptr)
        return;

    if (const auto *gateway = GetModule<UGateway>()) {
        SPDLOG_TRACE("{:<20} - From Service[ID: {}, Name: {}] To Player[{}]",
         __FUNCTION__, GetServiceID(), GetServiceName(), pid);

        gateway->PostToPlayer(pid, task);
    }
}

void IServiceBase::SendToClient(int64_t pid, const shared_ptr<IPackage_Interface> &pkg) const {
    if (mState != EServiceState::RUNNING)
        return;

    if (pkg == nullptr)
        return;

    if (const auto *gateway = GetModule<UGateway>()) {
        SPDLOG_TRACE("{:<20} - From Service[ID: {}, Name: {}] To Player[{}]",
        __FUNCTION__, GetServiceID(), GetServiceName(), pid);

        pkg->SetSource(GetServiceID());
        pkg->SetTarget(CLIENT_TARGET_ID);

        gateway->SendToClient(pid, pkg);
    }
}

void IServiceBase::ListenEvent(const int event) const {
    if (mState == EServiceState::CREATED || mState == EServiceState::TERMINATED)
        return;

    if (mContext == nullptr)
        return;

    mContext->ListenEvent(event);
}

void IServiceBase::RemoveListener(const int event) const {
    if (mContext == nullptr)
        return;

    mContext->RemoveListener(event);
}

void IServiceBase::DispatchEvent(const shared_ptr<IEventParam_Interface> &event) const {
    if (mState == EServiceState::CREATED || mState == EServiceState::TERMINATED)
        return;

    if (mContext == nullptr)
        return;

    mContext->DispatchEvent(event);
}

int64_t IServiceBase::CreateTimer(const std::function<void(IServiceBase *)> &task, const int delay, const int rate) const {
    if (mState == EServiceState::INITIALIZED || mState == EServiceState::TERMINATED)
        return -1;

    if (mContext == nullptr)
        return -1;

    return mContext->CreateTimer(task, delay, rate);
}

void IServiceBase::CancelTimer(const int64_t tid) const {
    if (mContext == nullptr)
        return;

    mContext->CancelTimer(tid);
}

void IServiceBase::CancelAllTimers() const {
    if (mContext == nullptr)
        return;

    mContext->CancelAllTimers();
}

void IServiceBase::TryCreateLogger(const std::string &name) const {
    if (mState <= EServiceState::CREATED || mState >= EServiceState::TERMINATED)
        return;

    auto *module = GetModule<ULoggerModule>();
    if (module == nullptr)
        return;

    module->TryCreateLogger(name);
}

EServiceState IServiceBase::GetState() const {
    return mState;
}

bool IServiceBase::Initial(const IDataAsset_Interface *pData) {
    if (mState != EServiceState::CREATED)
        return false;

    if (mContext == nullptr) {
        SPDLOG_ERROR("{:<20} - Context Is Null", __FUNCTION__);
        return false;
    }

    mState = EServiceState::INITIALIZED;
    return true;
}

awaitable<bool> IServiceBase::AsyncInitial(const IDataAsset_Interface *pData) {
    if (mState != EServiceState::CREATED)
        co_return false;

    if (mContext == nullptr) {
        SPDLOG_ERROR("{:<20} - Context Is Null", __FUNCTION__);
        co_return false;
    }

    mState = EServiceState::INITIALIZED;
    co_return true;
}

bool IServiceBase::Start() {
    mState = EServiceState::RUNNING;
    return true;
}

void IServiceBase::Stop() {
    if (mState == EServiceState::TERMINATED)
        return;

    mState = EServiceState::TERMINATED;
}

void IServiceBase::OnPackage(const shared_ptr<IPackage_Interface> &pkg) {
}

void IServiceBase::OnEvent(const shared_ptr<IEventParam_Interface> &event) {
}

void IServiceBase::OnUpdate(ASteadyTimePoint now, ASteadyDuration delta) {

}
