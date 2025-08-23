#include "ServiceBase.h"
#include "ServiceAgent.h"
#include "Server.h"
#include "base/Package.h"
#include "logger/LoggerModule.h"

#include <spdlog/spdlog.h>


IServiceBase::IServiceBase()
    : mState(EServiceState::CREATED),
      bUpdatePerTick(true) {
}

IServiceBase::~IServiceBase() {
}

std::string IServiceBase::GetServiceName() const {
    return {};
}

int64_t IServiceBase::GetServiceID() const {
    return GetAgentT<UServiceAgent>()->GetServiceID();
}

FPackageHandle IServiceBase::BuildPackage() const {
    if (mState != EServiceState::RUNNING)
        throw std::runtime_error("BuildPackage While Service Not In Running");

    if (auto pkg = GetAgent()->BuildPackage(); pkg.IsValid()) {
        pkg->SetSource(GetServiceID());
        return pkg;
    }

    return {};
}


void IServiceBase::ListenEvent(const int event) const {
    if (mState == EServiceState::CREATED || mState == EServiceState::TERMINATED)
        throw std::runtime_error(std::format("{} - In Error State: [{}]", __FUNCTION__, static_cast<int>(mState.load())));

    GetAgentT<UServiceAgent>()->ListenEvent(event);
}

void IServiceBase::RemoveListener(const int event) const {
    GetAgentT<UServiceAgent>()->RemoveListener(event);
}

void IServiceBase::DispatchEvent(const shared_ptr<IEventParam_Interface> &event) const {
    if (mState <= EServiceState::INITIALIZED)
        throw std::runtime_error(std::format("{} - Service Not Initialized", __FUNCTION__));

    GetAgentT<UServiceAgent>()->DispatchEvent(event);
}

FTimerHandle IServiceBase::CreateTimer(const ATimerTask &task, const int delay, const int rate) const {
    if (mState <= EServiceState::INITIALIZED || mState == EServiceState::TERMINATED)
        throw std::runtime_error(std::format("{} - In Error State: [{}]", __FUNCTION__, static_cast<int>(mState.load())));

    return GetAgent()->CreateTimer(task, delay, rate);
}

void IServiceBase::CancelTimer(const int64_t tid) const {
    GetAgent()->CancelTimer(tid);
}

void IServiceBase::CancelAllTimers() const {
    GetAgent()->CancelAllTimers();
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
    if (mState != EServiceState::CREATED)
        return false;

    mState = EServiceState::INITIALIZED;
    return true;
}

awaitable<bool> IServiceBase::AsyncInitial(const IDataAsset_Interface *pData) {
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

void IServiceBase::PostPackage(const FPackageHandle &pkg) const {
    if (pkg == nullptr)
        return;

    GetAgentT<UServiceAgent>()->PostPackage(pkg);
}

void IServiceBase::PostPackage(const std::string &name, const FPackageHandle &pkg) const {
    if (pkg == nullptr || name.empty())
        return;

    GetAgentT<UServiceAgent>()->PostPackage(name, pkg);
}

void IServiceBase::PostTask(const int64_t target, const AActorTask &task) const {
    if (target < 0 || task == nullptr)
        return;

    GetAgentT<UServiceAgent>()->PostTask(target, task);
}

void IServiceBase::PostTask(const std::string &name, const AActorTask &task) const {
    if (name.empty() || task == nullptr)
        return;

    GetAgentT<UServiceAgent>()->PostTask(name, task);
}

void IServiceBase::SendToPlayer(const int64_t pid, const FPackageHandle &pkg) const {
    if (pid <= 0 || pkg == nullptr)
        return;

    GetAgentT<UServiceAgent>()->SendToPlayer(pid, pkg);
}

void IServiceBase::PostToPlayer(const int64_t pid, const AActorTask &task) const {
    if (pid <= 0 || task == nullptr)
        return;

    GetAgentT<UServiceAgent>()->PostToPlayer(pid, task);
}

void IServiceBase::SendToClient(const int64_t pid, const FPackageHandle &pkg) const {
    if (pid <= 0 || pkg == nullptr)
        return;

    GetAgentT<UServiceAgent>()->SendToClient(pid, pkg);
}

void IServiceBase::OnPackage(IPackage_Interface *pkg) {
}

void IServiceBase::OnEvent(IEventParam_Interface *event) {
}

void IServiceBase::OnUpdate(ASteadyTimePoint now, ASteadyDuration delta) {

}
