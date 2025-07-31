#include "ServiceBase.h"
#include "ContextBase.h"
#include "Base/Packet.h"
#include "Service/ServiceModule.h"
#include "Service/ServiceContext.h"
#include "Gateway/Gateway.h"
#include "Event/EventModule.h"
#include "Timer/TimerModule.h"

#include <spdlog/spdlog.h>

void IServiceBase::SetUpContext(IContextBase *context) {
    if (mState != EServiceState::CREATED)
        return;
    mContext = context;
}

IContextBase *IServiceBase::GetContext() const {
    return mContext;
}

IServiceBase::IServiceBase()
    : mContext(nullptr),
      mState(EServiceState::CREATED) {
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

std::shared_ptr<FPacket> IServiceBase::BuildPackage() const {
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

void IServiceBase::PostPackage(const std::shared_ptr<FPacket> &pkg) const {
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

void IServiceBase::PostPackage(const std::string &name, const std::shared_ptr<FPacket> &pkg) const {
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
            __FUNCTION__, GetServiceID(), GetServiceName(), target->GetServiceID(), target->GetServiceName());

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
            __FUNCTION__, GetServiceID(), GetServiceName(), target->GetServiceID(), target->GetServiceName());

        target->PushTask(task);
    }
}

void IServiceBase::SendToPlayer(int64_t pid, const std::shared_ptr<FPacket> &pkg) const {
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

void IServiceBase::SendToClient(int64_t pid, const std::shared_ptr<FPacket> &pkg) const {
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
    if (auto *module = GetModule<UEventModule>()) {
        module->ListenEvent(event, GetServiceID());
    }
}

void IServiceBase::RemoveListener(int event) const {
    if (auto *module = GetModule<UEventModule>()) {
        module->RemoveListener(event, GetServiceID());
    }
}

void IServiceBase::DispatchEvent(const std::shared_ptr<IEventParam_Interface> &event) const {
    if (const auto *module = GetModule<UEventModule>()) {
        module->Dispatch(event);
    }
}

FTimerHandle IServiceBase::SetSteadyTimer(const std::function<void(IServiceBase *)> &task, int delay, int rate) const {
    if (auto *timer = GetModule<UTimerModule>()) {
        return timer->SetSteadyTimer(GetServiceID(), -1, task, delay, rate);
    }

    return { -1, true };
}

FTimerHandle IServiceBase::SetSystemTimer(const std::function<void(IServiceBase *)> &task, int delay, int rate) const {
    if (auto *timer = GetModule<UTimerModule>()) {
        return timer->SetSystemTimer(GetServiceID(), -1, task, delay, rate);
    }

    return { -1, false };
}

void IServiceBase::CancelTimer(const FTimerHandle &handle) {
    if (auto *timer = GetModule<UTimerModule>()) {
        if (handle.id > 0) {
            timer->CancelTimer(handle);
        } else {
            timer->CancelServiceTimer(GetServiceID());
        }
    }
}

EServiceState IServiceBase::GetState() const {
    return mState;
}

bool IServiceBase::Initial(const IDataAsset_Interface *data) {
    if (mState != EServiceState::CREATED)
        return false;

    if (mContext == nullptr) {
        SPDLOG_ERROR("{:<20} - Context Is Null", __FUNCTION__);
        return false;
    }
    mState = EServiceState::INITIALIZED;
    return true;
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

void IServiceBase::OnPackage(const std::shared_ptr<FPacket> &pkg) {
}

void IServiceBase::OnEvent(const std::shared_ptr<IEventParam_Interface> &event) {
}
