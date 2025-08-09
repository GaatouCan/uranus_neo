#include "PlayerComponent.h"
#include "ComponentModule.h"

IPlayerComponent::IPlayerComponent()
    : mModule(nullptr) {
}

IPlayerComponent::~IPlayerComponent() {
}

UPlayer *IPlayerComponent::GetPlayer() const {
    if (mModule) {
        return mModule->GetPlayer();
    }
    return nullptr;
}

int64_t IPlayerComponent::GetPlayerID() const {
    if (mModule) {
        return mModule->GetPlayerID();
    }
    return -1;
}

UServer *IPlayerComponent::GetServer() const {
    if (mModule) {
        return mModule->GetServer();
    }
    return nullptr;
}

void IPlayerComponent::OnLogin() {
}

void IPlayerComponent::OnLogout() {
}

void IPlayerComponent::OnDayChange() {
}

void IPlayerComponent::SetUpModule(UComponentModule *module) {
    mModule = module;
}
