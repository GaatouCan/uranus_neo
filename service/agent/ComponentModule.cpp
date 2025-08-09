#include "ComponentModule.h"
#include "Player.h"

UComponentModule::UComponentModule()
    : mPlayer(nullptr) {
}

UComponentModule::~UComponentModule() {
}

void UComponentModule::SetUpPlayer(UPlayer *player) {
    mPlayer = player;
}

UPlayer *UComponentModule::GetPlayer() const {
    return mPlayer;
}

int64_t UComponentModule::GetPlayerID() const {
    if (mPlayer) {
        return mPlayer->GetPlayerID();
    }
    return -1;
}

UServer *UComponentModule::GetServer() const {
    if (mPlayer) {
        return mPlayer->GetServer();
    }
    return nullptr;
}

void UComponentModule::OnLogin() {
    for (const auto &comp : mComponentMap | std::views::values) {
        comp->OnLogin();
    }
}

void UComponentModule::OnLogout() {
    for (const auto &comp : mComponentMap | std::views::values) {
        comp->OnLogout();
    }
}
