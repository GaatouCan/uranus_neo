#pragma once

#include "Base/Types.h"
#include "Base/SharedLibrary.h"


class IServiceBase;
class IModuleBase;
class IRecyclerBase;
class IEventParam_Interface;
class IDataAsset_Interface;
class IPackage_Interface;
class UServer;


enum class EContextState {
    CREATED,
    INITIALIZING,
    INITIALIZED,
    IDLE,
    RUNNING,
    WAITING,
    SHUTTING_DOWN,
    STOPPED
};

/**
 * Context For Interaction Between Service And The Server
 */
class BASE_API IContextBase : public std::enable_shared_from_this<IContextBase> {

    /**
     * The Base Channel Node For Internal Channel
     */
    class BASE_API ISchedule_Interface {

    public:
        ISchedule_Interface() = default;
        virtual ~ISchedule_Interface() = default;

        DISABLE_COPY_MOVE(ISchedule_Interface)
        virtual void Execute(IServiceBase *pService) = 0;
    };

    /**
     * The Wrapper Of Package,
     * While Service Received Package
     */
    class BASE_API UPackageNode final : public ISchedule_Interface {

        shared_ptr<IPackage_Interface> mPackage;

    public:
        void SetPackage(const shared_ptr<IPackage_Interface> &pkg);
        void Execute(IServiceBase *pService) override;
    };

    /**
     * The Wrapper Of Task,
     * While The Service Received The Task
     */
    class BASE_API UTaskNode final : public ISchedule_Interface {

        std::function<void(IServiceBase *)> mTask;

    public:
        void SetTask(const std::function<void(IServiceBase *)> &task);
        void Execute(IServiceBase *pService) override;
    };

    /**
     * The Wrapper Of Event,
     * While The Service Received Event Parameter
     */
    class BASE_API UEventNode final : public ISchedule_Interface {

        shared_ptr<IEventParam_Interface> mEvent;

    public:
        void SetEventParam(const shared_ptr<IEventParam_Interface> &event);
        void Execute(IServiceBase *pService) override;
    };

    class BASE_API UTickerNode final : public ISchedule_Interface {

        ASystemTimePoint mTickTime;
        ASystemDuration mDeltaTime;

    public:
        UTickerNode();

        void SetCurrentTickTime(ASystemTimePoint timepoint);
        void SetDeltaTime(ASystemDuration delta);

        void Execute(IServiceBase *pService) override;
    };

    using AContextChannel = TConcurrentChannel<void(std::error_code, std::unique_ptr<ISchedule_Interface>)>;

public:
    IContextBase();
    virtual ~IContextBase();

    DISABLE_COPY_MOVE(IContextBase)

    void SetUpModule(IModuleBase *pModule);
    void SetUpLibrary(const FSharedLibrary &library);

    virtual bool Initial(const IDataAsset_Interface *pData);
    virtual awaitable<bool> AsyncInitial(const IDataAsset_Interface *pData);

    virtual int Shutdown(bool bForce, int second, const std::function<void(IContextBase *)> &func);
    int ForceShutdown();

    bool BootService();

    [[nodiscard]] std::string GetServiceName() const;
    [[nodiscard]] virtual int32_t GetServiceID() const = 0;

    [[nodiscard]] UServer *GetServer() const;
    [[nodiscard]] IModuleBase *GetOwnerModule() const;

    [[nodiscard]] shared_ptr<IPackage_Interface> BuildPackage() const;

    [[nodiscard]] IServiceBase *GetOwningService() const;
    [[nodiscard]] EContextState GetState() const;

    void PushPackage(const shared_ptr<IPackage_Interface> &pkg);
    void PushTask(const std::function<void(IServiceBase *)> &task);
    void PushEvent(const shared_ptr<IEventParam_Interface> &event);
    void PushTicker(ASystemTimePoint timepoint, ASystemDuration delta);

private:
    awaitable<void> ProcessChannel();

private:
    /** The Owner Module */
    IModuleBase *mModule;

    /** The Owning Service */
    IServiceBase *mService;

    /** Loaded Library With Creator And Destroyer Of Service */
    FSharedLibrary mLibrary;

    /** Internal Package Pool */
    shared_ptr<IRecyclerBase> mPackagePool;

    /** Internal Node Channel */
    unique_ptr<AContextChannel> mChannel;

    /** When Timeout, Force Shut Down This Context */
    unique_ptr<ASteadyTimer> mShutdownTimer;

    /** Invoked While This Context Stopped */
    std::function<void(IContextBase *)> mShutdownCallback;

protected:
    std::atomic<EContextState> mState;
};
