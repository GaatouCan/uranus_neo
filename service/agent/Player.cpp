#include "Player.h"
#include "Server.h"

#include <config/Config.h>


UPlayer::UPlayer() {

    mComponentModule.SetUpPlayer(this);
    mConfig.LoadConfig(GetServer()->GetModule<UConfig>());
    LoadProtocol();
}

UPlayer::~UPlayer() {

}

void UPlayer::Initial() {

}

void UPlayer::OnLogin() {
    mComponentModule.OnLogin();
}

void UPlayer::OnLogout() {
    mComponentModule.OnLogout();
}

void UPlayer::Save() {

}

void UPlayer::OnReset() {

}

void UPlayer::OnEvent(IEventParam_Interface *event) {

}

void UPlayer::RegisterProtocol(const uint32_t id, const AProtocolFunctor &func) {
    mRoute.Register(id, func);
}

void UPlayer::OnPackage(IPackage_Interface *pkg) {
    if (pkg == nullptr)
        return;

    const auto pkt = dynamic_cast<FPacket *>(pkg);
    if (pkt == nullptr)
        return;

    mRoute.OnReceivePackage(pkt, this);
}

UComponentModule &UPlayer::GetComponentModule() {
    return mComponentModule;
}

extern "C" {
    SERVICE_API IPlayerBase *CreatePlayer() {
        return new UPlayer();
    }

    SERVICE_API void DestroyPlayer(IPlayerBase *inst) {
        delete dynamic_cast<UPlayer *>(inst);
    }
}
