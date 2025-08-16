#include "ServiceBase.h"
#include "ContextBase.h"
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
        throw std::runtime_error(std::format("{} - Context Is Still NULL Pointer", __FUNCTION__));
    return mContext->GetServer();
}

FPackageHandle IServiceBase::BuildPackage() const {
    if (mState != EServiceState::RUNNING)
        throw std::runtime_error("BuildPackage While Service Not In Running");

    if (mContext == nullptr)
        throw std::runtime_error("Service's Context Is NULL Pointer");

    if (auto pkg = mContext->BuildPackage()) {
        pkg->SetSource(GetServiceID());
        return pkg;
    }

    return {};
}

void IServiceBase::PostPackage(const FPackageHandle &pkg) const {
    if (mState <= EServiceState::INITIALIZED)
        throw std::runtime_error(std::format("{} - Service Not Initialized", __FUNCTION__));

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

void IServiceBase::PostPackage(const std::string &name, const FPackageHandle &pkg) const {
    if (mState <= EServiceState::INITIALIZED)
        throw std::runtime_error(std::format("{} - Service Not Initialized", __FUNCTION__));

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

void IServiceBase::PostTask(const int64_t target, const std::function<void(IServiceBase *)> &task) const {
    if (mState <= EServiceState::INITIALIZED)
        throw std::runtime_error(std::format("{} - Service Not Initialized", __FUNCTION__));

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
    if (mState <= EServiceState::INITIALIZED)
        throw std::runtime_error(std::format("{} - Service Not Initialized", __FUNCTION__));

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

void IServiceBase::SendToPlayer(const int64_t pid, const FPackageHandle &pkg) const {
    if (mState <= EServiceState::INITIALIZED)
        throw std::runtime_error(std::format("{} - Service Not Initialized", __FUNCTION__));

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

void IServiceBase::PostToPlayer(const int64_t pid, const std::function<void(IServiceBase *)> &task) const {
    if (mState <= EServiceState::INITIALIZED)
        throw std::runtime_error(std::format("{} - Service Not Initialized", __FUNCTION__));

    if (task == nullptr)
        return;

    if (const auto *gateway = GetModule<UGateway>()) {
        SPDLOG_TRACE("{:<20} - From Service[ID: {}, Name: {}] To Player[{}]",
            __FUNCTION__, GetServiceID(), GetServiceName(), pid);

        gateway->PostToPlayer(pid, task);
    }
}

void IServiceBase::SendToClient(const int64_t pid, const FPackageHandle &pkg) const {
    if (mState <= EServiceState::INITIALIZED)
        throw std::runtime_error(std::format("{} - Service Not Initialized", __FUNCTION__));

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
        throw std::runtime_error(std::format("{} - In Error State: [{}]", __FUNCTION__, static_cast<int>(mState.load())));

    if (mContext == nullptr)
        throw std::runtime_error(std::format("{} - Service 's Context Is NULL", __FUNCTION__));

    mContext->ListenEvent(event);
}

void IServiceBase::RemoveListener(const int event) const {
    if (mContext == nullptr)
        return;

    mContext->RemoveListener(event);
}

void IServiceBase::DispatchEvent(const shared_ptr<IEventParam_Interface> &event) const {
    if (mState <= EServiceState::INITIALIZED)
        throw std::runtime_error(std::format("{} - Service Not Initialized", __FUNCTION__));

    if (mContext == nullptr)
        throw std::runtime_error(std::format("{} - Service 's Context Is NULL", __FUNCTION__));

    mContext->DispatchEvent(event);
}

int64_t IServiceBase::CreateTimer(const std::function<void(IServiceBase *)> &task, const int delay, const int rate) const {
    if (mState <= EServiceState::INITIALIZED || mState == EServiceState::TERMINATED)
        throw std::runtime_error(std::format("{} - In Error State: [{}]", __FUNCTION__, static_cast<int>(mState.load())));

    if (mContext == nullptr)
        throw std::runtime_error(std::format("{} - Service 's Context Is NULL", __FUNCTION__));

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
    if (mState == EServiceState::CREATED || mState >= EServiceState::TERMINATED)
        throw std::runtime_error(std::format("{} - In Error State: [{}]", __FUNCTION__, static_cast<int>(mState.load())));

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
        throw std::runtime_error(std::format("{} - In Error State: [{}]", __FUNCTION__, static_cast<int>(mState.load())));

    if (mContext == nullptr)
        throw std::runtime_error(std::format("{} - Service 's Context Is NULL", __FUNCTION__));

    mState = EServiceState::INITIALIZED;
    return true;
}

awaitable<bool> IServiceBase::AsyncInitial(const IDataAsset_Interface *pData) {
    if (mState != EServiceState::CREATED)
        throw std::runtime_error(std::format("{} - In Error State: [{}]", __FUNCTION__, static_cast<int>(mState.load())));

    if (mContext == nullptr)
        throw std::runtime_error(std::format("{} - Service 's Context Is NULL", __FUNCTION__));

    mState = EServiceState::INITIALIZED;
    co_return true;
}

bool IServiceBase::Start() {
    if (mState != EServiceState::INITIALIZED)
        throw std::runtime_error(std::format("{} - In Error State: [{}]", __FUNCTION__, static_cast<int>(mState.load())));

    mState = EServiceState::RUNNING;
    return true;
}

void IServiceBase::Stop() {
    if (mState == EServiceState::TERMINATED)
        throw std::runtime_error(std::format("{} - In Error State: [{}]", __FUNCTION__, static_cast<int>(mState.load())));

    mState = EServiceState::TERMINATED;
}

void IServiceBase::OnPackage(IPackage_Interface *pkg) {
}

void IServiceBase::OnEvent(IEventParam_Interface *event) {
}

void IServiceBase::OnUpdate(ASteadyTimePoint now, ASteadyDuration delta) {

}
