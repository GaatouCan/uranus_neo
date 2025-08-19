#pragma once

#include "base/Types.h"
#include "base/Package.h"
#include "base/Recycler.h"


class IPackageCodec_Interface;
class UNetwork;
class UServer;

using FPackageHandle = FRecycleHandle<IPackage_Interface>;
using std::unique_ptr;


class BASE_API UConnection final : public std::enable_shared_from_this<UConnection> {

    friend class UNetwork;

    using APackageChannel = TConcurrentChannel<void(std::error_code, FPackageHandle)>;

public:
    UConnection() = delete;

    explicit UConnection(unique_ptr<IPackageCodec_Interface> &&codec);
    ~UConnection();

    [[nodiscard]] ATcpSocket &GetSocket() const;
    [[nodiscard]] bool IsSocketOpen() const;
    [[nodiscard]] APackageChannel &GetChannel();

    [[nodiscard]] UNetwork *GetNetwork() const;
    [[nodiscard]] UServer *GetServer() const;

    void SetExpireSecond(int sec);

    void ConnectToClient();
    void Disconnect();

    FPackageHandle BuildPackage() const;

    asio::ip::address RemoteAddress() const;
    [[nodiscard]] const std::string &GetKey() const;

    void SendPackage(const FPackageHandle &pkg);

private:
    void SetUpModule(UNetwork *network);

    awaitable<void> WritePackage();
    awaitable<void> ReadPackage();
    awaitable<void> Watchdog();

private:
    UNetwork *mNetwork;

    unique_ptr<IPackageCodec_Interface> mPackageCodec;
    unique_ptr<IRecyclerBase> mPackagePool;

    APackageChannel mChannel;

    ASteadyTimer mWatchdog;
    ASteadyTimePoint mReceiveTime;
    ASteadyDuration mExpiration;

    std::string mKey;
    std::atomic<int64_t> mPlayerID;
};

