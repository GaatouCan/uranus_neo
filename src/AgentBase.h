#pragma once

#include "ActorBase.h"
#include "base/Recycler.h"
#include "timer/TimerManager.h"


using std::unique_ptr;
using std::shared_ptr;
using std::make_shared;
using std::make_unique;

class UServer;
class IAgentBase;
class IActorBase;
class IPackage_Interface;
class IEventParam_Interface;

using FPackageHandle = FRecycleHandle<IPackage_Interface>;


class BASE_API IChannelNode_Interface {

public:
    IChannelNode_Interface() = default;
    virtual ~IChannelNode_Interface() = default;

    DISABLE_COPY_MOVE(IChannelNode_Interface)

    virtual void Execute(IActorBase *pActor) const = 0;
};

class BASE_API UChannelPackageNode final : public IChannelNode_Interface {

    FPackageHandle mPackage;

public:
    void SetPackage(const FPackageHandle &pkg);
    void Execute(IActorBase *pActor) const override;
};

class BASE_API UChannelEventNode final : public IChannelNode_Interface {

    std::shared_ptr<IEventParam_Interface> mEvent;

public:
    void SetEventParam(const std::shared_ptr<IEventParam_Interface> &event);
    void Execute(IActorBase *pActor) const override;
};

class BASE_API UChannelTaskNode final : public IChannelNode_Interface {

    std::function<void(IActorBase *)> mTask;

public:
    void SetTask(const std::function<void(IActorBase *)> &task);
    void Execute(IActorBase *pActor) const override;
};


class BASE_API IAgentBase : public std::enable_shared_from_this<IAgentBase> {

    using AChannel = TConcurrentChannel<void(std::error_code, unique_ptr<IChannelNode_Interface>)>;

public:
    IAgentBase() = delete;

    explicit IAgentBase(asio::io_context &ctx);
    virtual ~IAgentBase();

    DISABLE_COPY_MOVE(IAgentBase)

    [[nodiscard]] asio::io_context &GetIOContext() const;
    [[nodiscard]] UServer *GetServer() const;

    virtual void Initial(UServer *pServer);
    virtual void CleanUp();

    virtual void Start();
    virtual void Stop();

    void PushPackage(const FPackageHandle &pkg);
    void PushEvent(const shared_ptr<IEventParam_Interface> &event);
    void PushTask(const std::function<void(IActorBase *)> &task);

    FPackageHandle BuildPackage() const;

#pragma region Timer
    FTimerHandle CreateTimer(const ATimerTask &task, int delay, int rate = -1);
    void CancelTimer(int64_t tid) const;
    void CancelAllTimers();
#pragma endregion

protected:
    [[nodiscard]] virtual IActorBase *GetActor() const = 0;

private:
    awaitable<void> ProcessChannel();

private:
    asio::io_context &mContext;
    UServer *mServer;

protected:
    unique_ptr<AChannel> mChannel;
    unique_ptr<IRecyclerBase> mPackagePool;

    UTimerManager mTimerManager;
};
