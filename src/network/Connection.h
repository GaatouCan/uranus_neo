#pragma once

#include "base/Types.h"
#include "base/Package.h"
#include "base/RecycleHandle.h"



class IPackageCodec_Interface;

using FPackageHandle = FRecycleHandle<IPackage_Interface>;
using std::unique_ptr;


class BASE_API UConnection final : public std::enable_shared_from_this<UConnection> {

    using APackageChannel = TConcurrentChannel<void(std::error_code, FPackageHandle)>;

    unique_ptr<IPackageCodec_Interface> mCodec;
    APackageChannel mChannel;

    ASteadyTimer mWatchdog;
    ASteadyTimePoint mReceiveTime;
    ASteadyDuration mExpiration;

    std::string mKey;
    std::atomic<int64_t> mPlayerID;

public:
    UConnection() = delete;

    explicit UConnection(unique_ptr<IPackageCodec_Interface> &&codec);
    ~UConnection();

    [[nodiscard]] ATcpSocket &GetSocket() const;
    [[nodiscard]] bool IsSocketOpen() const;
    [[nodiscard]] APackageChannel &GetChannel();

    void SetExpireSecond(int sec);

    void ConnectToClient();
    void Disconnect();

    FPackageHandle BuildPackage() const;

    asio::ip::address RemoteAddress() const;
    [[nodiscard]] const std::string &GetKey() const;

    void SendPackage(const FPackageHandle &pkg);

private:
    awaitable<void> WritePackage();
    awaitable<void> ReadPackage();
    awaitable<void> Watchdog();
};

