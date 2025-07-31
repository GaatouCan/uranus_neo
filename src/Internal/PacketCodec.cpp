#include "PacketCodec.h"

UPacketCodec::UPacketCodec(ASslStream stream)
    : mStream(std::move(stream)) {
}

awaitable<bool> UPacketCodec::EncodeT(const shared_ptr<FPacket> &pkg) {
    co_return true;
}

awaitable<bool> UPacketCodec::DecodeT(const shared_ptr<FPacket> &pkg) {
    co_return true;
}
