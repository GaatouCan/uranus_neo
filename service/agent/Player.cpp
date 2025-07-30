#include "Player.h"

UPlayer::UPlayer() {

    LoadProtocol();
}

UPlayer::~UPlayer() {
}

extern "C" {
    SERVICE_API IServiceBase *CreateInstance() {
        return new UPlayer();
    }

    SERVICE_API void DestroyInstance(IServiceBase *inst) {
        delete dynamic_cast<UPlayer *>(inst);
    }
}
