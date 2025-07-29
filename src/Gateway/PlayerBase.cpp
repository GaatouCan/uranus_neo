#include "PlayerBase.h"

UPlayerBase::UPlayerBase()
    : mComponentModule(this) {
}

UPlayerBase::~UPlayerBase() {
}

void UPlayerBase::RegisterProtocol(const uint32_t id, const APlayerProtocol &func) {
    mRoute.Register(id, func);
}

void UPlayerBase::OnPackage(const std::shared_ptr<FPackage> &pkg) {
    mRoute.OnReceivePackage(pkg, this);
}

UComponentModule &UPlayerBase::GetComponentModule() {
    return mComponentModule;
}

void UPlayerBase::OnLogin() {
    mComponentModule.OnLogin();
}

void UPlayerBase::OnLogout() {
    mComponentModule.OnLogout();
}

bool UPlayerBase::Start() {
    IPlayerAgent::Start();
    this->OnLogin();
    return true;
}

void UPlayerBase::Stop() {
    IPlayerAgent::Stop();
    this->OnLogout();
}
