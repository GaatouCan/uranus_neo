#pragma once

#include <Gateway/PlayerBase.h>


class UPlayer final : public UPlayerBase {

public:
    UPlayer();
    ~UPlayer() override;

private:
    void LoadProtocol();
};

#define REGISTER_PROTOCOL(type, func) \
    RegisterProtocol(static_cast<uint32_t>(protocol::EProtoType::##type), &protocol::##func);
