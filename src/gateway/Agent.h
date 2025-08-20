#pragma once

#include "base/Types.h"
#include "base/Package.h"
#include "base/Recycler.h"


class IPackageCodec_Interface;
class UGateway;
class UServer;
class IPlayerBase;

using FPackageHandle = FRecycleHandle<IPackage_Interface>;
using std::unique_ptr;
using std::shared_ptr;
using std::weak_ptr;

enum class EAgentState {
    CREATED,
    INITIALIZED,
    CONNECTED,
    IDLE,
    RUNNING,
    WAITING,
    TERMINATED,
};


class BASE_API UAgent final : public std::enable_shared_from_this<UAgent> {

#pragma region Schedule Node
    class BASE_API ISchedule_Interface {
    public:
        ISchedule_Interface () = default;
        virtual ~ISchedule_Interface () = default;

        virtual void Execute(IPlayerBase *pPlayer) const = 0;
    };
#pragma endregion

    using APackageChannel = TConcurrentChannel<void(std::error_code, FPackageHandle)>;
    using AScheduleChannel = TConcurrentChannel<void(std::error_code, unique_ptr<ISchedule_Interface>)>;

public:
    UAgent() = delete;

    explicit UAgent(unique_ptr<IPackageCodec_Interface> &&codec);
    ~UAgent();

    [[nodiscard]] ATcpSocket &GetSocket() const;
    [[nodiscard]] bool IsSocketOpen() const;
    [[nodiscard]] asio::ip::address RemoteAddress() const;
    [[nodiscard]] const std::string &GetKey() const;

    [[nodiscard]] UGateway *GetGateway() const;
    [[nodiscard]] UServer *GetServer() const;

    void SetUpModule(UGateway *network);
    void SetExpireSecond(int sec);

    FPackageHandle BuildPackage() const;

    void ConnectToClient();
    void Disconnect();

    void OnLogin(unique_ptr<IPlayerBase> &&plr);
    [[nodiscard]] unique_ptr<IPlayerBase> ExtractPlayer();

    void SendPackage(const FPackageHandle &pkg);

private:
    awaitable<void> WritePackage();
    awaitable<void> ReadPackage();
    awaitable<void> Watchdog();

    awaitable<void> ProcessChannel();

private:
    UGateway *mGateway;

    unique_ptr<IPackageCodec_Interface> mPackageCodec;
    unique_ptr<IRecyclerBase> mPackagePool;

    APackageChannel mPackageChannel;
    AScheduleChannel mScheduleChannel;

    ASteadyTimer mWatchdog;
    ASteadyTimePoint mReceiveTime;
    ASteadyDuration mExpiration;

    unique_ptr<IPlayerBase> mPlayer;

    std::string mKey;
};

