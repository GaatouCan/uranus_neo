#include "Connection.h"
#include "Base/Packet.h"
#include "Network.h"
#include "Server.h"
#include "Login/LoginAuth.h"
#include "Gateway/Gateway.h"
#include "Utils.h"

#include <asio/experimental/awaitable_operators.hpp>
#include <spdlog/spdlog.h>

#if defined(_WIN32) || defined(_WIN64)
#include <WinSock2.h>
#else
#include <arpa/inet.h>
#include <endian.h>
#endif


using namespace asio::experimental::awaitable_operators;
using namespace std::literals::chrono_literals;


UConnection::UConnection(ASslStream stream)
    : mStream(std::move(stream)),
      mChannel(mStream.next_layer().get_executor(), 1024),
      mNetwork(nullptr),
      mWatchdog(mStream.next_layer().get_executor()),
      mExpiration(std::chrono::seconds(30)),
      mPlayerID(-1) {
    mID = static_cast<int64_t>(mStream.next_layer().native_handle());
    mStream.next_layer().set_option(asio::ip::tcp::no_delay(true));
    mStream.next_layer().set_option(asio::ip::tcp::socket::keep_alive(true));
}

void UConnection::SetUpModule(UNetwork *owner) {
    mNetwork = owner;
}

UConnection::~UConnection() {
    Disconnect();
}

bool UConnection::IsSocketOpen() const {
    return mStream.next_layer().is_open();
}

ATcpSocket &UConnection::GetSocket() {
    return mStream.next_layer();
}

UConnection::APackageChannel &UConnection::GetChannel() {
    return mChannel;
}

void UConnection::SetExpireSecond(const int sec) {
    mExpiration = std::chrono::seconds(sec);
}

void UConnection::SetPlayerID(const int64_t id) {
    if (mPlayerID > 0)
        return;

    mPlayerID = id;
}

void UConnection::ConnectToClient() {
    mReceiveTime = std::chrono::steady_clock::now();

    co_spawn(GetSocket().get_executor(), [self = shared_from_this()]() -> awaitable<void> {
        if (const auto [ec] = co_await self->mStream.async_handshake(asio::ssl::stream_base::server); ec) {
            SPDLOG_ERROR("Connection[{}] Handshake Failed: {}", self->RemoteAddress().to_string(), ec.message());
            co_return;
        }

        co_await (
            self->ReadPackage() ||
            self->WritePackage() ||
            self->Watchdog()
        );
    }, detached);
}

void UConnection::Disconnect() {
    if (!IsSocketOpen())
        return;

    mWatchdog.cancel();
    GetSocket().close();

    mNetwork->RemoveConnection(mID, mPlayerID);

    if (mPlayerID > 0) {
        mPlayerID = -1;
    }
}

UNetwork *UConnection::GetNetworkModule() const {
    return mNetwork;
}

UServer *UConnection::GetServer() const {
    if (mNetwork)
        return mNetwork->GetServer();
    return nullptr;
}

shared_ptr<FPacket> UConnection::BuildPackage() const {
    if (mNetwork)
        return mNetwork->BuildPackage();
    return nullptr;
}

asio::ip::address UConnection::RemoteAddress() const {
    if (IsSocketOpen()) {
        return mStream.next_layer().remote_endpoint().address();
    }
    return {};
}

int64_t UConnection::GetConnectionID() const {
    return mID;
}

int64_t UConnection::GetPlayerID() const {
    return mPlayerID;
}

void UConnection::SendPackage(const shared_ptr<FPacket> &pkg) {
    if (pkg == nullptr)
        return;

    co_spawn(GetSocket().get_executor(), [self = shared_from_this(), pkg]() -> awaitable<void> {
        co_await self->mChannel.async_send(std::error_code{}, pkg);
    }, detached);
}


awaitable<void> UConnection::WritePackage() {
    if (GetServer() == nullptr) {
        SPDLOG_CRITICAL("{:<20} - Use GetServer() Before Set Up!", __FUNCTION__);
        co_return;
    }

    try {
        while (IsSocketOpen()) {
            const auto [ec, pkg] = co_await mChannel.async_receive();
            if (ec || pkg == nullptr)
                co_return;

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

            if (pkg->mHeader.length > 0) {

                const auto buffers = {
                    asio::buffer(&header, FPacket::PACKAGE_HEADER_SIZE),
                    asio::buffer(pkg->mPayload.RawRef()),
                };

                if (const auto [ec, len] = co_await async_write(mStream, buffers); ec || len == 0) {
                    if (ec)
                        SPDLOG_WARN("{:<20} - Failed To Write Package, Error Code: {}", __FUNCTION__, ec.message());
                    if (len == 0)
                        SPDLOG_WARN("{:<20} - Package Length Equal Zero", __FUNCTION__);

                    Disconnect();
                    break;
                }

            } else {
                if (const auto [ec, len] = co_await async_write(mStream, asio::buffer(&header, FPacket::PACKAGE_HEADER_SIZE)); ec || len == 0) {
                    if (ec)
                        SPDLOG_WARN("{:<20} - Failed To Write Package, Error Code: {}", __FUNCTION__, ec.message());
                    if (len != FPacket::PACKAGE_HEADER_SIZE)
                        SPDLOG_WARN("{:<20} - Write Package Header Length Not Equal", __FUNCTION__);

                    Disconnect();
                    break;
                }
            }

            // Can Do Something Here
        }
    } catch (const std::exception &e) {
        SPDLOG_ERROR("{:<20} - {}", __FUNCTION__, e.what());
        Disconnect();
    }
}

awaitable<void> UConnection::ReadPackage() {
    if (GetServer() == nullptr) {
        SPDLOG_CRITICAL("{:<20} - Use GetServer() Before Set Up!", __FUNCTION__);
        co_return;
    }

    try {
        while (IsSocketOpen()) {
            const auto pkg = BuildPackage();
            if (pkg == nullptr)
                co_return;

            if (const auto [ec, len] = co_await async_read(mStream, asio::buffer(&pkg->mHeader, FPacket::PACKAGE_HEADER_SIZE)); ec || len == 0) {
                if (ec)
                    SPDLOG_WARN("{:<20} -  Failed To Read Package Header, Error Code: {}", __FUNCTION__, ec.message());
                if (len != FPacket::PACKAGE_HEADER_SIZE)
                    SPDLOG_WARN("{:<20} - Read Package Header Length Not Equal", __FUNCTION__);

                Disconnect();
                break;
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

            if (pkg->mHeader.length > 0) {
                pkg->mPayload.Reserve(pkg->mHeader.length);
                if (const auto [ec, len] = co_await async_read(mStream, asio::buffer(pkg->RawRef())); ec || len != pkg->mHeader.length) {
                    if (ec)
                        SPDLOG_WARN("{:<20} - Fail To Read Package Payload, {}", __FUNCTION__, ec.message());
                    if (len != pkg->mHeader.length)
                        SPDLOG_WARN("{:<20} - The Payload Length Not Equal", __FUNCTION__);

                    Disconnect();
                    break;
                }
            }

            const auto now = std::chrono::steady_clock::now();

            // Run The Login Branch
            if (mPlayerID < 0) {
                // Do Not Try Login Too Frequently
                if (now - mReceiveTime > std::chrono::seconds(3)) {
                    --mPlayerID;

                    // Try Login Failed 3 Times Then Disconnect This
                    if (mPlayerID < -3) {
                        SPDLOG_WARN("{:<20} - Connection[{}] Try Login Too Many Times", __FUNCTION__, RemoteAddress().to_string());
                        Disconnect();
                        break;
                    }

                    // Handle Login Logic
                    if (auto *login = GetServer()->GetModule<ULoginAuth>(); login != nullptr) {
                        login->OnPlayerLogin(mID, pkg);
                    }
                }

                mReceiveTime = now;
                continue;
            }

            // Update Receive Time Point For Watchdog
            mReceiveTime = now;

            if (const auto *gateway = GetServer()->GetModule<UGateway>(); gateway != nullptr) {
                if (pkg->GetPackageID() == 1001) {
                    gateway->OnHeartBeat(mPlayerID, pkg);
                } else {
                    gateway->OnClientPackage(mPlayerID, pkg);
                }
            }
        }
    } catch (const std::exception &e) {
        SPDLOG_ERROR("{:<20} - {}", __FUNCTION__, e.what());
        Disconnect();
    }
}

awaitable<void> UConnection::Watchdog() {
    if (mExpiration == ASteadyDuration::zero())
        co_return;

    try {
        ASteadyTimePoint now;

        do {
            mWatchdog.expires_at(mReceiveTime + mExpiration);

            if (auto [ec] = co_await mWatchdog.async_wait(); ec) {
                SPDLOG_DEBUG("{:<20} - Timer Canceled.", __FUNCTION__);
                co_return;
            }

            now = std::chrono::steady_clock::now();
        } while ((mReceiveTime + mExpiration) > now);

        if (IsSocketOpen()) {
            SPDLOG_WARN("{:<20} - Watchdog Timeout", __FUNCTION__);
            Disconnect();
        }
    } catch (const std::exception &e) {
        SPDLOG_ERROR("{:<20} - {}", __FUNCTION__, e.what());
    }
}
