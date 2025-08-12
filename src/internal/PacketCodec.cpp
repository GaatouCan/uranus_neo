#include "PacketCodec.h"

#include <spdlog/spdlog.h>
#if defined(_WIN32) || defined(_WIN64)
#include <WinSock2.h>
#else
#include <arpa/inet.h>
#include <endian.h>
#endif

UPacketCodec::UPacketCodec(ASslStream stream)
    : mStream(std::move(stream)) {
}

awaitable<bool> UPacketCodec::Initial() {
    if (const auto [ec] = co_await mStream.async_handshake(asio::ssl::stream_base::server); ec) {
        SPDLOG_ERROR("Connection[{}] Handshake Failed: {}",
            mStream.next_layer().remote_endpoint().address().to_string(), ec.message());
        co_return false;
    }
    co_return true;
}

awaitable<bool> UPacketCodec::EncodeT(const shared_ptr<FPacket> &pkg) {
    FPacket::FHeader header{};
    memset(&header, 0, sizeof(FPacket::FHeader));

    header.magic = htonl(pkg->mHeader.magic);
    header.id = htonl(pkg->mHeader.id);

    header.source = static_cast<int32_t>(htonl(pkg->mHeader.source));
    header.target = static_cast<int32_t>(htonl(pkg->mHeader.target));

#if defined(_WIN32) || defined(_WIN64)
    header.length = htonll(pkg->mHeader.length);
#else
    header.length = htobe64(pkg->mHeader.length);
#endif

    if (pkg->mHeader.length <= 0) {
        const auto [ec, len] = co_await async_write(mStream, asio::buffer(&header, FPacket::PACKAGE_HEADER_SIZE));

        if (ec) {
            SPDLOG_WARN("{:<20} - Failed To Write Packet Header, Error Code: {}", __FUNCTION__, ec.message());
            co_return false;
        }

        if (len != FPacket::PACKAGE_HEADER_SIZE) {
            SPDLOG_WARN("{:<20} - Length Of Written Packet Header Incorrect, {}", __FUNCTION__, len);
            co_return false;
        }

        co_return true;
    }

    if (pkg->mHeader.length > 4096 * 1024)
        co_return false;

    const auto buffers = {
        asio::buffer(&header, FPacket::PACKAGE_HEADER_SIZE),
        asio::buffer(pkg->mPayload.RawRef()),
    };

    const auto [ec, len] = co_await async_write(mStream, buffers);

    if (ec) {
        SPDLOG_WARN("{:<20} - Failed To Write Packet, Error Code: {}", __FUNCTION__, ec.message());
        co_return false;
    }

    if (len <= FPacket::PACKAGE_HEADER_SIZE) {
        SPDLOG_WARN("{:<20} - Length Of Written Packet Incorrect, {}", __FUNCTION__, len);
        co_return false;
    }

    co_return true;
}

awaitable<bool> UPacketCodec::DecodeT(const shared_ptr<FPacket> &pkg) {
    if (const auto [ec, len] = co_await async_read(mStream, asio::buffer(&pkg->mHeader, FPacket::PACKAGE_HEADER_SIZE));
        ec || len == 0) {
        if (ec) {
            SPDLOG_WARN("{:<20} -  Failed To Read Packet Header, Error Code: {}", __FUNCTION__, ec.message());
            co_return false;
        }

        if (len != FPacket::PACKAGE_HEADER_SIZE) {
            SPDLOG_WARN("{:<20} - Length Of Read Packet Header Incorrect, {}", __FUNCTION__, len);
            co_return false;
        }
    }

    pkg->mHeader.magic = ntohl(pkg->mHeader.magic);
    pkg->mHeader.id = ntohl(pkg->mHeader.id);

    pkg->mHeader.source = static_cast<int32_t>(ntohl(pkg->mHeader.source));
    pkg->mHeader.target = static_cast<int32_t>(ntohl(pkg->mHeader.target));

#if defined(_WIN32) || defined(_WIN64)
    pkg->mHeader.length = ntohll(pkg->mHeader.length);
#else
    pkg->mHeader.length = be64toh(pkg->mHeader.length);
#endif

    if (pkg->mHeader.length <= 0)
        co_return true;

    // Payload Too Long
    if (pkg->mHeader.length > 4096 * 1024)
        co_return false;

    pkg->mPayload.Reserve(pkg->mHeader.length);
    const auto [ec, len] = co_await async_read(mStream, asio::buffer(pkg->RawRef()));

    if (ec) {
        SPDLOG_WARN("{:<20} - Failed To Read Packet, Error Code: {}", __FUNCTION__, ec.message());
        co_return false;
    }

    if (len < pkg->mHeader.length) {
        SPDLOG_WARN("{:<20} - Length Of Read Packet Incorrect, {}", __FUNCTION__, len);
        co_return false;
    }

    co_return true;
}

ATcpSocket &UPacketCodec::GetSocket() {
    return mStream.next_layer();
}
