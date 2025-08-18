#pragma once

#include "base/Types.h"
#include "base/Package.h"
#include "base/SharedLibrary.h"
#include "base/ContextHandle.h"
#include "base/RecycleHandle.h"


class UServer;
class IServiceBase;
class IModuleBase;
class IRecyclerBase;
class IEventParam_Interface;
class IDataAsset_Interface;

using FPackageHandle = FRecycleHandle<IPackage_Interface>;
using std::unique_ptr;


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
 * The Context Between Services And The Server.
 * Used To Manage Independent Resources Of A Single Service,
 * And Exchange Data Between Inner Service And The Server.
 */
class BASE_API UContextBase : public std::enable_shared_from_this<UContextBase> {

#pragma region Schedule Wrapper

    /**
     * The Abstract Interface Of The Wrapper Of The Schedulable Things In Context Channel.
     * Used To Wrap The Data Which Send To Inner Service
     */
    class BASE_API ISchedule_Interface {

    public:
        ISchedule_Interface() = default;
        virtual ~ISchedule_Interface() = default;

        DISABLE_COPY_MOVE(ISchedule_Interface)

        /** Call The Service Method To Handle Data **/
        virtual void Execute(IServiceBase *pService) = 0;
    };

    /**
     * The Wrapper Of Package
     */
    class BASE_API UPackageNode final : public ISchedule_Interface {

        FPackageHandle mPackage;

    public:
        void SetPackage(const FPackageHandle &pkg);
        void Execute(IServiceBase *pService) override;
    };


    /**
     * The Wrapper Of Task
     */
    class BASE_API UTaskNode final : public ISchedule_Interface {

        std::function<void(IServiceBase *)> mTask;

    public:
        void SetTask(const std::function<void(IServiceBase *)> &task);
        void Execute(IServiceBase *pService) override;
    };


    /**
     * The Wrapper Of Event
     */
    class BASE_API UEventNode final : public ISchedule_Interface {

        shared_ptr<IEventParam_Interface> mEvent;

    public:
        void SetEventParam(const shared_ptr<IEventParam_Interface> &event);
        void Execute(IServiceBase *pService) override;
    };

    /**
     * The Wrapper Of Ticker
     */
    class BASE_API UTickerNode final : public ISchedule_Interface {

        ASteadyTimePoint    mTickTime;
        ASteadyDuration     mDeltaTime;

    public:
        UTickerNode();

        void SetCurrentTickTime (ASteadyTimePoint   timepoint);
        void SetDeltaTime       (ASteadyDuration    delta);

        void Execute(IServiceBase *pService) override;
    };
#pragma endregion

    using AContextChannel = TConcurrentChannel<void(std::error_code, unique_ptr<ISchedule_Interface>)>;

public:
    UContextBase();
    virtual ~UContextBase();

    DISABLE_COPY_MOVE(UContextBase)

    void SetUpModule    (IModuleBase *          pModule );
    void SetUpServiceID (FServiceHandle         sid     );
    void SetUpLibrary   (const FSharedLibrary&  library );

    /** Get The Service Name, It Must Be Unique **/
    [[nodiscard]] std::string       GetServiceName()    const;

    /** Get The Servie ID, It Must Be Unique **/
    [[nodiscard]] FServiceHandle    GetServiceID()      const;

    [[nodiscard]] UServer *     GetServer()         const;
    [[nodiscard]] IModuleBase * GetOwnerModule()    const;

    /** Generate A Handle To Represent Itself **/
    [[nodiscard]] FContextHandle GenerateHandle();

    virtual bool            Initial     (const IDataAsset_Interface *pData);
    virtual awaitable<bool> AsyncInitial(const IDataAsset_Interface *pData);

    /**
     * Shut Down The Service
     * @param bForce If Force To Shut Down
     * @param second If Not Force, How Many Seconds To Wait
     * @param func   If Not Force, It Will Be Call After Waiting And Shutdown
     * @return Negative Means Error Happened, Zero Means Change To Waiting, One Means Shut Down Success Immediately
     */
    virtual int Shutdown(bool bForce, int second, const std::function<void(UContextBase *)> &func);

    int ForceShutdown();

    bool BootService();

    [[nodiscard]] FPackageHandle BuildPackage() const;

    [[nodiscard]] IServiceBase *GetOwningService() const;
    [[nodiscard]] EContextState GetState() const;

#pragma region Push
    void PushPackage(const FPackageHandle&                          pkg     );
    void PushTask   (const std::function<void(IServiceBase *)>&     task    );
    void PushEvent  (const shared_ptr<IEventParam_Interface>&       event   );
    void PushTicker (ASteadyTimePoint timepoint, ASteadyDuration    delta   );
#pragma endregion

#pragma region Timer
    int64_t CreateTimer(const std::function<void(IServiceBase *)> &task, int delay, int rate = -1);
    void    CancelTimer(int64_t tid) const;
    void    CancelAllTimers();
#pragma endregion

#pragma region Event
    void ListenEvent    (int event);
    void RemoveListener (int event);

    void DispatchEvent(const shared_ptr<IEventParam_Interface> &param) const;
#pragma endregion

private:
    awaitable<void> ProcessChannel();

private:
    /** The Owner Module **/
    IModuleBase *mModule;

    /** The Unique ID Of Inner Service **/
    FServiceHandle mServiceID;

    /** Inner Service **/
    IServiceBase *mService;

    /** The Package Pool For Inner Service To Build A Package **/
    unique_ptr<IRecyclerBase> mPackagePool;

    /** The Internal Channel To Schedule Data **/
    unique_ptr<AContextChannel> mChannel;

    /** Loaded Library With Creator And Destroyer Of Service */
    FSharedLibrary mLibrary;

    /** The Timer To Wait To Force Shutdown After Delay **/
    unique_ptr<ASteadyTimer> mShutdownTimer;

    /** It Will Be Invoke After The Service Shutdown, Only In Not Force Shut Down **/
    std::function<void(UContextBase *)> mShutdownCallback;

protected:
    /** Current Context State **/
    std::atomic<EContextState> mState;
};
