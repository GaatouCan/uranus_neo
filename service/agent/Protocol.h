#pragma once

#include <memory>

class FPacket;
class UPlayer;

namespace protocol {
    void AppearanceRequest(uint32_t, const std::shared_ptr<FPacket> &, UPlayer *);
}
