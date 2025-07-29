#include "ComponentModule.h"
#include "PlayerBase.h"

UComponentModule::UComponentModule(UPlayerBase *player)
    : mOwner(player) {
}

UComponentModule::~UComponentModule() {
}

UPlayerBase *UComponentModule::GetPlayer() const {
    return mOwner;
}

int64_t UComponentModule::GetPlayerID() const {
    return mOwner->GetPlayerID();
}

void UComponentModule::OnLogin() {
    for (const auto &type : mComponentOrder) {
        if (const auto it = mComponents.find(type); it != mComponents.end()) {
            it->second->OnLogin();
        }
    }
}

void UComponentModule::OnLogout() {
    for (const auto &type : mComponentOrder) {
        if (const auto it = mComponents.find(type); it != mComponents.end()) {
            it->second->OnLogout();
        }
    }
}
