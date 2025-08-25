#include "Player.h"
#include "Protocol.h"

#include <ProtoType.gen.h>

void UPlayer::LoadProtocol() {
    // REGISTER_PROTOCOL(APPEARANCE_REQUEST, AppearanceRequest);
    RegisterProtocol(static_cast<uint32_t>(protocol::EProtoType::APPEARANCE_REQUEST), &protocol::AppearanceRequest);
}