#pragma once

#include "Common.h"
#include "Base/Types.h"

#include <memory>


struct evp_cipher_ctx_st;
typedef evp_cipher_ctx_st EVP_CIPHER_CTX;

class UNetwork;
class UServer;
class FPackage;

using std::shared_ptr;

class BASE_API UConnection final : public std::enable_shared_from_this<UConnection> {

    using APackageChannel = TConcurrentChannel<void(std::error_code, shared_ptr<FPackage>)>;

    ATcpSocket mSocket;
    APackageChannel mChannel;

    UNetwork *mNetwork;

    EVP_CIPHER_CTX *mEncryptContext;

    ASteadyTimer mWatchdog;
    ASteadyTimePoint mReceiveTime;
    ASteadyDuration mExpiration;

    int64_t mID;
    std::atomic_int64_t mPlayerID;

protected:
    explicit UConnection(ATcpSocket socket);

    void SetUpModule(UNetwork *owner);

public:
    UConnection() = delete;
    ~UConnection();

    [[nodiscard]] ATcpSocket &GetSocket();
    [[nodiscard]] APackageChannel &GetChannel();


    void SetExpireSecond(int sec);
    void SetPlayerID(int64_t id);

    void ConnectToClient();
    void Disconnect();

    [[nodiscard]] UNetwork *GetNetworkModule() const;
    [[nodiscard]] UServer *GetServer() const;

    shared_ptr<FPackage> BuildPackage() const;

    asio::ip::address RemoteAddress() const;
    [[nodiscard]] int64_t GetConnectionID() const;
    [[nodiscard]] int64_t GetPlayerID() const;

    void SendPackage(const shared_ptr<FPackage> &pkg);

private:
    awaitable<void> WritePackage();
    awaitable<void> ReadPackage();
    awaitable<void> Watchdog();

};

