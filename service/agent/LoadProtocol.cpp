#include "Player.h"
#include "Protocol.h"

#include <ProtoType.gen.h>

void UPlayer::LoadProtocol() {
    REGISTER_PROTOCOL(APPEARANCE_REQUEST, AppearanceRequest);
}