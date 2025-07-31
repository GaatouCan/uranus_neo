#pragma once

#include "Base/PackageCodec.h"
#include "Base/Packet.h"
#include "Base/Types.h"

class BASE_API UPacketCodec final : public TPackageCodec<FPacket> {

    ASslStream mStream;

public:
    UPacketCodec() = delete;

    explicit UPacketCodec(ASslStream stream);
    ~UPacketCodec() override = default;

    awaitable<bool> EncodeT(const shared_ptr<FPacket> &pkg) override;
    awaitable<bool> DecodeT(const shared_ptr<FPacket> &pkg) override;
};

