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
