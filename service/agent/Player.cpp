#include "Player.h"

#include <Config/Config.h>

UPlayer::UPlayer() {
    LoadProtocol();

    mConfig.LoadConfig(GetModule<UConfig>());
}

UPlayer::~UPlayer() {
}

void UPlayer::RegisterProtocol(const uint32_t id, const AProtocolFunctor &func) {
    mRoute.Register(id, func);
}

void UPlayer::OnPackage(const std::shared_ptr<IPackage_Interface> &pkg) {
    if (pkg == nullptr)
        return;

    const auto pkt = std::dynamic_pointer_cast<FPacket>(pkg);
    if (pkt == nullptr)
        return;

    mRoute.OnReceivePackage(pkt, this);
}

extern "C" {
    SERVICE_API IServiceBase *CreateInstance() {
        return new UPlayer();
    }

    SERVICE_API void DestroyInstance(IServiceBase *inst) {
        delete dynamic_cast<UPlayer *>(inst);
    }
}
