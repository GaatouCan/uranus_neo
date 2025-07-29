#include "PlayerComponent.h"
#include "ComponentModule.h"

IPlayerComponent::IPlayerComponent()
    : mModule(nullptr) {
}

IPlayerComponent::~IPlayerComponent() {
}

void IPlayerComponent::SetUpModule(UComponentModule *module) {
    mModule = module;
}

UPlayerBase *IPlayerComponent::GetPlayer() const {
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

void IPlayerComponent::OnLogin() {
}

void IPlayerComponent::OnLogout() {
}
