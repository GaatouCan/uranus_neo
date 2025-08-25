#pragma once

#include "ActorBase.h"
#include "base/Recycler.h"
#include "timer/TimerManager.h"


class UServer;
class IModuleBase;
class IAgentBase;
class IActorBase;
class IPackage_Interface;
class IEventParam_Interface;
class IDataAsset_Interface;

using std::unique_ptr;
using std::shared_ptr;
using std::make_shared;
using std::make_unique;

using FPackageHandle = FRecycleHandle<IPackage_Interface>;
using AActorTask = std::function<void(IActorBase *)>;

/**
 * The Interface That Stored In Agent Inner Channel
 */
class BASE_API IChannelNode_Interface {

public:
    IChannelNode_Interface() = default;
    virtual ~IChannelNode_Interface() = default;

    DISABLE_COPY_MOVE(IChannelNode_Interface)

    /// Implement This Method To Execute The Specific Task
    virtual void Execute(IActorBase *pActor) const = 0;
};

/**
 * Wrapper Of The Package
 */
class BASE_API UChannelPackageNode final : public IChannelNode_Interface {

    FPackageHandle mPackage;

public:
    void SetPackage(const FPackageHandle &pkg);
    void Execute(IActorBase *pActor) const override;
};

/**
 * Wrapper Of The Event Parameter
 */
class BASE_API UChannelEventNode final : public IChannelNode_Interface {

    shared_ptr<IEventParam_Interface> mEvent;

public:
    void SetEventParam(const shared_ptr<IEventParam_Interface> &event);
    void Execute(IActorBase *pActor) const override;
};

/**
 * Wrapper Of The Function
 */
class BASE_API UChannelTaskNode final : public IChannelNode_Interface {

    AActorTask mTask;

public:
    void SetTask(const AActorTask &task);
    void Execute(IActorBase *pActor) const override;
};

/**
 * The Basic Agent For Player And The Service.
 * There Is A Concurrent Channel, Timer Manager And Package Pool Inside
 */
class BASE_API IAgentBase : public std::enable_shared_from_this<IAgentBase> {

protected:
    using AChannel = TConcurrentChannel<void(std::error_code, unique_ptr<IChannelNode_Interface>)>;

    /** The Reference Of The IOContext That Drive The Whole Agent **/
    asio::io_context &mContext;

    /** The Pointer To The Directory Owner Module **/
    IModuleBase *mModule;

    /** The Inner Package Pool **/
    unique_ptr<IRecyclerBase> mPackagePool;

    /** The Inner Channel To Schedule The Nodes **/
    AChannel mChannel;

    /** The Inner Timer Manager **/
    UTimerManager mTimerManager;

public:
    IAgentBase() = delete;

    explicit IAgentBase(asio::io_context &context, size_t channelSize = SERVICE_CHANNEL_SIZE);
    virtual ~IAgentBase();

    DISABLE_COPY_MOVE(IAgentBase)

    /// Return The IO Thread's Context(PlayerAgent) Or The Worker Thread's Context(ServiceAgent)
    [[nodiscard]] asio::io_context &GetIOContext() const;

    /// Return The Pointer Of UServer
    [[nodiscard]] UServer *GetServer() const;

    /**
     * Override This Method In Derived Class.
     * Create The Channel And Package Pool Instance And Other Resource,
     * And Set The Detail Of Them
     */
    virtual bool Initial(IModuleBase *pModule, IDataAsset_Interface *pData);

    /// Push The Package To The Inner Channel
    void PushPackage(const FPackageHandle &pkg);

    /// Push The Event Parameter To The Inner Channel
    void PushEvent(const shared_ptr<IEventParam_Interface> &event);

    /// Push The Function To The Inner Channel
    void PushTask(const AActorTask &task);

    /// Return A Package Handle From The Inner Package Pool
    FPackageHandle BuildPackage() const;

    /**
     * Create A Timer Use Inner TimerManager
     * @param task      The Task Will Be Executed
     * @param delay     The First Execute Delay
     * @param rate      The Repeat Interval, If It Is Negative, The Task Will Execute Only Once Then The Timer Invalid
     * @return          The TimerHandle
     */
    FTimerHandle CreateTimer(const ATimerTask &task, int delay, int rate = -1);

    /// Cancel The Timer By Timer ID
    void CancelTimer(int64_t tid) const;

    /// Cancel All The Timers
    void CancelAllTimers();

protected:
    /// Get The Actor As The Execute Target In Channel Node
    [[nodiscard]] virtual IActorBase *GetActor() const = 0;

    /// Will Be Called At The End Of The ::ProcessChannel(), You Can Release The Specific Resource Here
    virtual void CleanUp();

    /// Process The Node In Channel In A Looping Of The Coroutine
    awaitable<void> ProcessChannel();
};
