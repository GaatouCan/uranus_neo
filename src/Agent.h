#pragma once

#include "base/Types.h"
#include "base/Package.h"
#include "base/RecycleHandle.h"


class IPackageCodec_Interface;
class IEventParam_Interface;
class IAgentHandler;
class UServer;
class IPlayerBase;

using FPackageHandle = FRecycleHandle<IPackage_Interface>;
using std::unique_ptr;
using std::shared_ptr;
using std::weak_ptr;

using APlayerTask = std::function<void(IPlayerBase *)>;


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

    class BASE_API UPackageNode final : public ISchedule_Interface {

        FPackageHandle mPackage;

    public:
        ~UPackageNode () override = default;

        void SetPackage(const FPackageHandle &package);
        void Execute(IPlayerBase *pPlayer) const override;
    };

    class BASE_API UEventNode final : public ISchedule_Interface {

        std::shared_ptr<IEventParam_Interface> mEvent;

    public:
        ~UEventNode () override = default;

        void SetEventParam(const std::shared_ptr<IEventParam_Interface> &event);
        void Execute(IPlayerBase *pPlayer) const override;
    };

    class BASE_API UTaskNode final : public ISchedule_Interface {

        APlayerTask mTask;

    public:
        ~UTaskNode () override = default;

        void SetTask(const APlayerTask &task);
        void Execute(IPlayerBase *pPlayer) const override;
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

    [[nodiscard]] UServer *GetServer() const;

    void SetUpAgent(UServer *pServer);
    void SetExpireSecond(int sec);

    FPackageHandle BuildPackage() const;

    void ConnectToClient();
    void Disconnect();

    void SetUpPlayer(unique_ptr<IPlayerBase> &&plr);
    [[nodiscard]] unique_ptr<IPlayerBase> ExtractPlayer();

#pragma region Schedule Node
    void PushPackage(const FPackageHandle &pkg);
    void PushEvent(const std::shared_ptr<IEventParam_Interface> &event);
    void PushTask(const APlayerTask &task);
#pragma endregion

    void SendPackage(const FPackageHandle &pkg);

    void OnLoginFailed(const std::string &desc);
    void OnRepeated(const std::string &addr);

private:
    awaitable<void> WritePackage();
    awaitable<void> ReadPackage();
    awaitable<void> Watchdog();

    awaitable<void> ProcessChannel();
    void CleanUp();

private:
    UServer *mServer;

    unique_ptr<IPackageCodec_Interface> mPackageCodec;

    unique_ptr<IAgentHandler> mHandler;
    unique_ptr<IRecyclerBase> mPackagePool;

    APackageChannel mPackageChannel;
    AScheduleChannel mScheduleChannel;

    ASteadyTimer mWatchdog;
    ASteadyTimePoint mReceiveTime;
    ASteadyDuration mExpiration;

    unique_ptr<IPlayerBase> mPlayer;

    std::string mKey;
    bool bCached;
};

