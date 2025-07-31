#pragma once

#include "Common.h"
#include "Base/Types.h"

#include <memory>

class UNetwork;
class UServer;
class FPacket;

using std::shared_ptr;


class BASE_API UConnection final : public std::enable_shared_from_this<UConnection> {

    using APackageChannel = TConcurrentChannel<void(std::error_code, shared_ptr<FPacket>)>;
    friend class UNetwork;

    // ATcpSocket mSocket;
    ASslStream mStream;
    APackageChannel mChannel;

    UNetwork *mNetwork;

    ASteadyTimer mWatchdog;
    ASteadyTimePoint mReceiveTime;
    ASteadyDuration mExpiration;

    int64_t mID;
    std::atomic_int64_t mPlayerID;

protected:
    void SetUpModule(UNetwork *owner);

public:
    UConnection() = delete;

    explicit UConnection(ASslStream stream);
    ~UConnection();

    [[nodiscard]] ATcpSocket &GetSocket();
    [[nodiscard]] bool IsSocketOpen() const;
    [[nodiscard]] APackageChannel &GetChannel();

    void SetExpireSecond(int sec);
    void SetPlayerID(int64_t id);

    void ConnectToClient();
    void Disconnect();

    [[nodiscard]] UNetwork *GetNetworkModule() const;
    [[nodiscard]] UServer *GetServer() const;

    shared_ptr<FPacket> BuildPackage() const;

    asio::ip::address RemoteAddress() const;
    [[nodiscard]] int64_t GetConnectionID() const;
    [[nodiscard]] int64_t GetPlayerID() const;

    void SendPackage(const shared_ptr<FPacket> &pkg);

private:
    awaitable<void> WritePackage();
    awaitable<void> ReadPackage();
    awaitable<void> Watchdog();
};

