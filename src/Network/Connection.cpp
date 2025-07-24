#include "Connection.h"
#include "Base/Package.h"
#include "Network.h"

#include <openssl/evp.h>
#include <openssl/rand.h>
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


UConnection::UConnection(ATcpSocket socket)
    : mSocket(std::move(socket)),
      mChannel(mSocket.get_executor(), 1024),
      mNetwork(nullptr),
      mWatchdog(mSocket.get_executor()),
      mExpiration(std::chrono::seconds(30)),
      mPlayerID(-1) {
    mEncryptContext = EVP_CIPHER_CTX_new();
    mID = static_cast<int64_t>(mSocket.native_handle());
}

void UConnection::SetUpModule(UNetwork *owner) {
    mNetwork = owner;
}

UConnection::~UConnection() {
    Disconnect();
    EVP_CIPHER_CTX_free(mEncryptContext);
}

ATcpSocket &UConnection::GetSocket() {
    return mSocket;
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

    co_spawn(mSocket.get_executor(), [self = shared_from_this()]() -> awaitable<void> {
        co_await (
            self->ReadPackage() ||
            self->WritePackage() ||
            self->Watchdog()
        );
    }, detached);
}

void UConnection::Disconnect() {
    if (!mSocket.is_open())
        return;

    mWatchdog.cancel();
    mSocket.close();

    // mNetwork->RemoveConnection(mID, mPlayerID);

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

shared_ptr<FPackage> UConnection::BuildPackage() const {
    if (mNetwork)
        return mNetwork->BuildPackage();
    return nullptr;
}

asio::ip::address UConnection::RemoteAddress() const {
    if (mSocket.is_open()) {
        return mSocket.remote_endpoint().address();
    }
    return {};
}

int64_t UConnection::GetConnectionID() const {
    return mID;
}

int64_t UConnection::GetPlayerID() const {
    return mPlayerID;
}

void UConnection::SendPackage(const shared_ptr<FPackage> &pkg) {
    if (pkg == nullptr)
        return;

    co_spawn(mSocket.get_executor(), [self = shared_from_this(), pkg]() -> awaitable<void> {
        co_await self->mChannel.async_send(std::error_code{}, pkg);
    }, detached);
}

awaitable<void> UConnection::WritePackage() {
    try {
        while (mSocket.is_open()) {
            const auto [ec, pkg] = co_await mChannel.async_receive();
            if (ec || pkg == nullptr)
                co_return;

            FPackage::FHeader header{};
            memset(&header, 0, sizeof(FPackage::FHeader));

            header.magic = htonl(pkg->mHeader.magic);
            header.id = htonl(pkg->mHeader.id);

            header.source = static_cast<int32_t>(htonl(pkg->mHeader.source));
            header.target = static_cast<int32_t>(htonl(pkg->mHeader.target));

            if (pkg->mHeader.length > 0) {
                std::array<uint8_t, 16> salt{};
                if (RAND_bytes(salt.data(), salt.size()) != 1) {
                    SPDLOG_WARN("{:<20} - Failed To Generate iv", __FUNCTION__);
                    continue;
                }

                int length = 0;
                size_t encryptedLength = 0;
                std::vector<uint8_t> encrypted(pkg->mPayload.Size() + 16);

                EVP_EncryptUpdate(mEncryptContext, encrypted.data(), &length, pkg->mPayload.Data(), pkg->mPayload.Size());
                encryptedLength = length;
                EVP_EncryptFinal_ex(mEncryptContext, encrypted.data() + length, &length);
                encryptedLength += length;
                encrypted.resize(encryptedLength);

#if defined(_WIN32) || defined(_WIN64)
                header.length = htonll(encryptedLength);
#else
                header.length = htobe64(encryptedLength);
#endif

                const auto buffers = {
                    asio::buffer(&header, FPackage::PACKAGE_HEADER_SIZE),
                    asio::buffer(salt),
                    asio::buffer(encrypted)
                };

                const auto [ec, len] = co_await async_write(mSocket, buffers);

                if (ec) {
                    SPDLOG_WARN("{:<20} - Failed To Write Package, Error Code: {}", __FUNCTION__, ec.message());
                    Disconnect();
                    break;
                }

                if (len == 0)
                    SPDLOG_WARN("{:<20} - Package Length Equal Zero", __FUNCTION__);
            } else {
#if defined(_WIN32) || defined(_WIN64)
                header.length = htonll(pkg->mHeader.length);
#else
                header.length = htobe64(pkg->mHeader.length);
#endif

                const auto [ec, len] = co_await async_write(mSocket, asio::buffer(&header, FPackage::PACKAGE_HEADER_SIZE));

                if (ec) {
                    SPDLOG_WARN("{:<20} - Failed To Write Package, Error Code: {}", __FUNCTION__, ec.message());
                    Disconnect();
                    break;
                }

                if (len == 0)
                    SPDLOG_WARN("{:<20} - Package Header Length Equal Zero", __FUNCTION__);
            }

            // Can Do Something Here
        }
    } catch (const std::exception &e) {
        SPDLOG_ERROR("{:<20} - {}", __FUNCTION__, e.what());
        Disconnect();
    }
}

awaitable<void> UConnection::ReadPackage() {
    try {
        while (mSocket.is_open()) {
            const auto pkg = BuildPackage();
            if (pkg == nullptr)
                co_return;

            const auto [ec, len] = co_await async_read(mSocket, asio::buffer(&pkg->mHeader, FPackage::PACKAGE_HEADER_SIZE));

            if (ec) {
                SPDLOG_WARN("{:<20} - {}", __FUNCTION__, ec.message());
                Disconnect();
                break;
            }

            if (len == 0) {
                SPDLOG_WARN("{:<20} - Package Header Length Equal Zero", __FUNCTION__);
                continue;
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
                std::array<uint8_t, 16> salt{};
                const auto [saltEc, saltLen] = co_await async_read(mSocket, asio::buffer(salt));

                if (saltEc) {
                    SPDLOG_WARN("{:<20} - Failed To Read Salt, Error Code: {}", __FUNCTION__, saltEc.message());
                    Disconnect();
                    break;
                }

                if (saltLen != 16) {
                    SPDLOG_WARN("{:<20} - Read Salt Length Not Equal 16", __FUNCTION__);
                    Disconnect();
                    break;
                }

                std::vector<uint8_t> ciphertext(pkg->mHeader.length);

                const auto [ec, len] = co_await async_read(mSocket, asio::buffer(ciphertext));

                if (ec) {
                    SPDLOG_WARN("{:<20} - Failed To Read Package Payload, Error Code: {}", __FUNCTION__, ec.message());
                    Disconnect();
                    break;
                }

                if (len != pkg->mHeader.length) {
                    SPDLOG_WARN("{:<20} - Read Package Payload Length Not Equal", __FUNCTION__);
                    Disconnect();
                    break;
                }


                int length = 0;
                int decryptedLength = 0;

                pkg->mPayload.Reserve(len);
                memset(pkg->mPayload.Data(), 0, len);

                EVP_DecryptUpdate(mEncryptContext, pkg->mPayload.Data(), &length, ciphertext.data(), ciphertext.size());
                decryptedLength = length;
                EVP_DecryptFinal_ex(mEncryptContext, pkg->mPayload.Data() + len, &length);
                decryptedLength += length;

                pkg->mPayload.Resize(decryptedLength);
            }

            const auto now = std::chrono::steady_clock::now();

            // Run The Login Branch
            // if (mPlayerID < 0) {
            //     // Do Not Try Login Too Frequently
            //     if (now - mReceiveTime > std::chrono::seconds(3)) {
            //         --mPlayerID;
            //
            //         // Try Login Failed 3 Times Then Disconnect This
            //         if (mPlayerID < -3) {
            //             SPDLOG_WARN("{:<20} - Connection[{}] Try Login Too Many Times", __FUNCTION__, RemoteAddress().to_string());
            //             Disconnect();
            //             break;
            //         }
            //
            //         // Handle Login Logic
            //         if (auto *login = GetServer()->GetModule<ULoginAuth>(); login != nullptr) {
            //             login->OnPlayerLogin(mId, pkg);
            //         }
            //     }
            //
            //     mReceiveTime = now;
            //     continue;
            // }

            // Update Receive Time Point For Watchdog
            mReceiveTime = now;

            // if (const auto *gateway = GetServer()->GetModule<UGateway>(); gateway != nullptr) {
            //     if (pkg->GetPackageID() == 1001) {
            //         gateway->OnHeartBeat(mPlayerID, pkg);
            //     } else {
            //         gateway->OnClientPackage(mPlayerID, pkg);
            //     }
            // }
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

        if (mSocket.is_open()) {
            SPDLOG_WARN("{:<20} - Watchdog Timeout", __FUNCTION__);
            Disconnect();
        }
    } catch (const std::exception &e) {
        SPDLOG_ERROR("{:<20} - {}", __FUNCTION__, e.what());
    }
}
