#include "ServiceBase.h"
#include "Context.h"
#include "Server.h"
#include "base/Package.h"
#include "logger/LoggerModule.h"

#include <spdlog/spdlog.h>


void IServiceBase::SetUpContext(UContext *pContext) {
    if (mState != EServiceState::CREATED)
        return;
    mContext = pContext;
}

UContext *IServiceBase::GetContext() const {
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

int64_t IServiceBase::GetServiceID() const {
    if (mContext == nullptr)
        return INVALID_SERVICE_ID;
    return mContext->GetServiceID();
}

asio::io_context &IServiceBase::GetIOContext() const {
    return mContext->GetIOContext();
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

    if (auto pkg = mContext->BuildPackage(); pkg.IsValid()) {
        pkg->SetSource(GetServiceID());
        return pkg;
    }

    return {};
}

void IServiceBase::PostPackage(const FPackageHandle &pkg) const {
    if (mState <= EServiceState::INITIALIZED)
        throw std::runtime_error(std::format("{} - Service Not Initialized", __FUNCTION__));

    mContext->PostPackage(pkg);
}

void IServiceBase::PostPackage(const std::string &name, const FPackageHandle &pkg) const {
    if (mState <= EServiceState::INITIALIZED)
        throw std::runtime_error(std::format("{} - Service Not Initialized", __FUNCTION__));

    mContext->PostPackage(name, pkg);
}

void IServiceBase::PostTask(const int64_t target, const std::function<void(IServiceBase *)> &task) const {
    if (mState <= EServiceState::INITIALIZED)
        throw std::runtime_error(std::format("{} - Service Not Initialized", __FUNCTION__));

    mContext->PostTask(target, task);
}

void IServiceBase::PostTask(const std::string &name, const std::function<void(IServiceBase *)> &task) const {
    if (mState <= EServiceState::INITIALIZED)
        throw std::runtime_error(std::format("{} - Service Not Initialized", __FUNCTION__));

    mContext->PostTask(name, task);
}

void IServiceBase::SendToPlayer(const int64_t pid, const FPackageHandle &pkg) const {
    if (mState <= EServiceState::INITIALIZED)
        throw std::runtime_error(std::format("{} - Service Not Initialized", __FUNCTION__));

   mContext->SendToPlayer(pid, pkg);
}

void IServiceBase::PostToPlayer(const int64_t pid, const APlayerTask &task) const {
    if (mState <= EServiceState::INITIALIZED)
        throw std::runtime_error(std::format("{} - Service Not Initialized", __FUNCTION__));

    mContext->PostToPlayer(pid, task);
}

void IServiceBase::SendToClient(const int64_t pid, const FPackageHandle &pkg) const {
    if (mState <= EServiceState::INITIALIZED)
        throw std::runtime_error(std::format("{} - Service Not Initialized", __FUNCTION__));

    mContext->SendToClient(pid, pkg);
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

FTimerHandle IServiceBase::CreateTimer(const ATimerTask &task, const int delay, const int rate) const {
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

    auto *module = GetServer()->GetModule<ULoggerModule>();
    if (module == nullptr)
        return;

    module->TryCreateLogger(name);
}

EServiceState IServiceBase::GetState() const {
    return mState;
}

bool IServiceBase::Initial(const IDataAsset_Interface *pData) {
    if (mContext == nullptr)
        throw std::runtime_error(std::format("{} - Service 's Context Is NULL", __FUNCTION__));

    if (mState != EServiceState::CREATED)
        return false;

    mState = EServiceState::INITIALIZED;
    return true;
}

awaitable<bool> IServiceBase::AsyncInitial(const IDataAsset_Interface *pData) {
    if (mContext == nullptr)
        throw std::runtime_error(std::format("{} - Service 's Context Is NULL", __FUNCTION__));

    if (mState != EServiceState::CREATED)
        co_return false;

    mState = EServiceState::INITIALIZED;
    co_return true;
}

bool IServiceBase::Start() {
    if (mState != EServiceState::INITIALIZED)
        return false;

    mState = EServiceState::RUNNING;
    return true;
}

void IServiceBase::Stop() {
    if (mState == EServiceState::TERMINATED)
        return;

    mState = EServiceState::TERMINATED;
}

void IServiceBase::OnPackage(IPackage_Interface *pkg) {
}

void IServiceBase::OnEvent(IEventParam_Interface *event) {
}

void IServiceBase::OnUpdate(ASteadyTimePoint now, ASteadyDuration delta) {

}
