#pragma once

#include "Common.h"
#include "base/Types.h"


class UNetwork;
class UServer;
class IPackage_Interface;
class IPackageCodec_Interface;


class BASE_API UConnection final : public std::enable_shared_from_this<UConnection> {

    using APackageChannel = TConcurrentChannel<void(std::error_code, shared_ptr<IPackage_Interface>)>;
    friend class UNetwork;

    UNetwork *mNetwork;

    unique_ptr<IPackageCodec_Interface> mCodec;
    APackageChannel mChannel;

    ASteadyTimer mWatchdog;
    ASteadyTimePoint mReceiveTime;
    ASteadyDuration mExpiration;

    std::string mKey;
    std::atomic_int64_t mPlayerID;

protected:
    void SetUpModule(UNetwork *owner);

public:
    UConnection() = delete;

    explicit UConnection(IPackageCodec_Interface *codec);
    ~UConnection();

    [[nodiscard]] ATcpSocket &GetSocket() const;
    [[nodiscard]] bool IsSocketOpen() const;
    [[nodiscard]] APackageChannel &GetChannel();

    void SetExpireSecond(int sec);
    void SetPlayerID(int64_t id);

    void ConnectToClient();
    void Disconnect();

    [[nodiscard]] UNetwork *GetNetworkModule() const;
    [[nodiscard]] UServer *GetServer() const;

    shared_ptr<IPackage_Interface> BuildPackage() const;

    asio::ip::address RemoteAddress() const;
    [[nodiscard]] const std::string &GetKey() const;
    [[nodiscard]] int64_t GetPlayerID() const;

    void SendPackage(const shared_ptr<IPackage_Interface> &pkg);

private:
    awaitable<void> WritePackage();
    awaitable<void> ReadPackage();
    awaitable<void> Watchdog();
};

