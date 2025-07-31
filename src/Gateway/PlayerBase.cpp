#include "PlayerBase.h"

UPlayerBase::UPlayerBase()
    : mComponentModule(this) {
}

UPlayerBase::~UPlayerBase() {
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
