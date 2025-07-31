#pragma once

#include "Base/PackageCodec.h"
#include "Packet.h"
#include "Base/Types.h"

#include <asio/ssl/stream.hpp>


class BASE_API UPacketCodec final : public TPackageCodec<FPacket> {

    using ASslStream = asio::ssl::stream<ATcpSocket>;

    ASslStream mStream;

public:
    UPacketCodec() = delete;

    explicit UPacketCodec(ASslStream stream);
    ~UPacketCodec() override = default;

    awaitable<bool> Initial() override;

    awaitable<bool> EncodeT(const shared_ptr<FPacket> &pkg) override;
    awaitable<bool> DecodeT(const shared_ptr<FPacket> &pkg) override;

    ATcpSocket &GetSocket() override;
};

