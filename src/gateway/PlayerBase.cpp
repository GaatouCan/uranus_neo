#include "PlayerBase.h"
#include "PlayerAgent.h"
#include "base/Package.h"


IPlayerBase::IPlayerBase()
    : mPlayerID(-1) {
}

IPlayerBase::~IPlayerBase() {
}

int64_t IPlayerBase::GetPlayerID() const {
    return mPlayerID;
}

void IPlayerBase::Initial() {
}

void IPlayerBase::OnLogin() {
}

void IPlayerBase::OnLogout() {
}

void IPlayerBase::Save() {
    // TODO
}

void IPlayerBase::OnReset() {
}

void IPlayerBase::SendPackage(const FPackageHandle &pkg) const {
    if (pkg == nullptr)
        return;

    pkg->SetSource(PLAYER_TARGET_ID);
    pkg->SetTarget(CLIENT_TARGET_ID);

    GetAgentT<UPlayerAgent>()->SendPackage(pkg);
}

void IPlayerBase::PostPackage(const FPackageHandle &pkg) const {
    if (pkg == nullptr || pkg->GetTarget() < 0)
        return;
    GetAgentT<UPlayerAgent>()->PostPackage(pkg);
}

void IPlayerBase::PostPackage(const std::string &name, const FPackageHandle &pkg) const {
    if (pkg == nullptr || name.empty())
        return;

    GetAgentT<UPlayerAgent>()->PostPackage(name, pkg);
}

void IPlayerBase::PostTask(int64_t target, const AActorTask &task) const {
    if (target < 0 || task == nullptr)
        return;

    GetAgentT<UPlayerAgent>()->PostTask(target, task);
}

void IPlayerBase::PostTask(const std::string &name, const AActorTask &task) const {
    if (name.empty() || task == nullptr)
        return;

    GetAgentT<UPlayerAgent>()->PostTask(name, task);
}

FTimerHandle IPlayerBase::CreateTimer(const ATimerTask &task, const int delay, const int rate) const {
    return GetAgent()->CreateTimer(task, delay, rate);
}

void IPlayerBase::CancelTimer(const int64_t tid) const {
    GetAgent()->CancelTimer(tid);
}

void IPlayerBase::CancelAllTimers() const {
    GetAgent()->CancelAllTimers();
}

void IPlayerBase::SetPlayerID(const int64_t id) {
    mPlayerID = id;
}

void IPlayerBase::CleanAgent() {
    mAgent = nullptr;
}
