#include "Connection.h"
#include "Base/Package.h"
#include "Network.h"
#include "Server.h"
#include "Config/Config.h"
#include "Login/LoginAuth.h"
#include "Gateway/Gateway.h"
#include "Utils.h"

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
    mID = static_cast<int64_t>(mSocket.native_handle());
    mSocket.set_option(asio::ip::tcp::no_delay(true));
    mSocket.set_option(asio::ip::tcp::socket::keep_alive(true));
}

void UConnection::SetUpModule(UNetwork *owner) {
    mNetwork = owner;
}

UConnection::~UConnection() {
    Disconnect();
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
    if (GetServer() == nullptr) {
        SPDLOG_CRITICAL("{:<20} - Use GetServer() Before Set Up!", __FUNCTION__);
        co_return;
    }

    const auto *config = GetServer()->GetModule<UConfig>();
    if (config == nullptr) {
        SPDLOG_CRITICAL("{:<20} - Config Module Not Found!", __FUNCTION__);
        GetServer()->Shutdown();
        exit(-1);
    }

    const auto &cfg = config->GetServerConfig();
    const auto encryptionKey = cfg["server"]["encryption"]["key"].as<std::string>();

    const auto key = utils::HexToBytes(encryptionKey);

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
                std::array<uint8_t, 12> iv{};
                RAND_bytes(iv.data(), iv.size());

                std::vector<uint8_t> ciphertext(pkg->mPayload.Size());
                std::array<uint8_t, 16> tag{};

                EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
                EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr);
                EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv.size(), nullptr);
                EVP_EncryptInit_ex(ctx, nullptr, nullptr, key.data(), iv.data());

                int len = 0;
                EVP_EncryptUpdate(ctx, ciphertext.data(), &len, pkg->mPayload.Data(), static_cast<int>(pkg->mPayload.Size()));
                int total = len;

                EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len);
                total += len;

                EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, tag.size(), tag.data());
                EVP_CIPHER_CTX_free(ctx);

#if defined(_WIN32) || defined(_WIN64)
                header.length = htonll(total);
#else
                header.length = htobe64(total);
#endif

                const auto buffers = {
                    asio::buffer(&header, FPackage::PACKAGE_HEADER_SIZE),
                    asio::buffer(iv),
                    asio::buffer(ciphertext.data(), total),
                    asio::buffer(tag)
                };

                if (const auto [ec, len] = co_await async_write(mSocket, buffers); ec || len == 0) {
                    if (ec)
                        SPDLOG_WARN("{:<20} - Failed To Write Package, Error Code: {}", __FUNCTION__, ec.message());

                    if (len == 0)
                        SPDLOG_WARN("{:<20} - Package Length Equal Zero", __FUNCTION__);

                    Disconnect();
                    break;
                }

            } else {
#if defined(_WIN32) || defined(_WIN64)
                header.length = htonll(pkg->mHeader.length);
#else
                header.length = htobe64(pkg->mHeader.length);
#endif

                if (const auto [ec, len] = co_await async_write(mSocket, asio::buffer(&header, FPackage::PACKAGE_HEADER_SIZE)); ec || len == 0) {
                    if (ec)
                        SPDLOG_WARN("{:<20} - Failed To Write Package, Error Code: {}", __FUNCTION__, ec.message());

                    if (len == 0)
                        SPDLOG_WARN("{:<20} - Package Header Length Equal Zero", __FUNCTION__);

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

    const auto *config = GetServer()->GetModule<UConfig>();
    if (config == nullptr) {
        SPDLOG_CRITICAL("{:<20} - Config Module Not Found!", __FUNCTION__);
        GetServer()->Shutdown();
        exit(-1);
    }

    const auto &cfg = config->GetServerConfig();
    const auto encryptionKey = cfg["server"]["encryption"]["key"].as<std::string>();

    const auto key = utils::HexToBytes(encryptionKey);

    try {
        while (mSocket.is_open()) {
            const auto pkg = BuildPackage();
            if (pkg == nullptr)
                co_return;

            if (const auto [ec, len] = co_await async_read(mSocket, asio::buffer(&pkg->mHeader, FPackage::PACKAGE_HEADER_SIZE)); ec || len == 0) {
                if (ec)
                    SPDLOG_WARN("{:<20} -  Failed To Read Package Header, Error Code: {}", __FUNCTION__, ec.message());

                if (len == 0)
                    SPDLOG_WARN("{:<20} - Package Header Length Equal Zero", __FUNCTION__);

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
                std::array<uint8_t, 12> iv{};
                if (const auto [ec, len] = co_await async_read(mSocket, asio::buffer(iv)); ec || len != 12) {
                    if (ec)
                        SPDLOG_WARN("{:<20} -  Failed To Read Package IV, Error Code: {}", __FUNCTION__, ec.message());
                    if (len != 0)
                        SPDLOG_WARN("{:<20} - Package IV Length Not Equal 12", __FUNCTION__);
                    Disconnect();
                    break;
                }

                std::vector<uint8_t> ciphertext(pkg->mHeader.length);
                if (const auto [ec, len] = co_await async_read(mSocket, asio::buffer(ciphertext)); ec || len == 0) {
                    if (ec)
                        SPDLOG_WARN("{:<20} -  Failed To Read Package Cipher Text, Error Code: {}", __FUNCTION__, ec.message());
                    if (len != 0)
                        SPDLOG_WARN("{:<20} - Package Cipher Text Length Equal 0", __FUNCTION__);
                    Disconnect();
                    break;
                }

                std::array<uint8_t, 16> tag{};
                if (const auto [ec, len] = co_await async_read(mSocket, asio::buffer(tag)); ec || len != 16) {
                    if (ec)
                        SPDLOG_WARN("{:<20} -  Failed To Read Package Tag, Error Code: {}", __FUNCTION__, ec.message());
                    if (len != 0)
                        SPDLOG_WARN("{:<20} - Package Tag Length Not Equal 16", __FUNCTION__);
                    Disconnect();
                    break;
                }

                pkg->mPayload.Reserve(pkg->mHeader.length);

                EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
                EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr);
                EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv.size(), nullptr);
                EVP_DecryptInit_ex(ctx, nullptr, nullptr, key.data(), iv.data());
                EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, tag.size(), tag.data());

                int len = 0;
                EVP_DecryptUpdate(ctx, pkg->mPayload.Data(), &len, ciphertext.data(), static_cast<int>(ciphertext.size()));
                int total = len;

                const int ret = EVP_DecryptFinal_ex(ctx, pkg->mPayload.Data() + len, &len);
                EVP_CIPHER_CTX_free(ctx);

                if (ret <= 0) {
                    SPDLOG_WARN("{:<20} - GCM Tag Check Failed, Connection: {}", __FUNCTION__, RemoteAddress().to_string());
                    Disconnect();
                    co_return;
                }

                total += len;
                pkg->mPayload.Resize(total);
                pkg->mHeader.length = total;
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

        if (mSocket.is_open()) {
            SPDLOG_WARN("{:<20} - Watchdog Timeout", __FUNCTION__);
            Disconnect();
        }
    } catch (const std::exception &e) {
        SPDLOG_ERROR("{:<20} - {}", __FUNCTION__, e.what());
    }
}
