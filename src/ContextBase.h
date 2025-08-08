#pragma once

#include "Base/Types.h"
#include "Base/SharedLibrary.h"
#include "Base/ContextHandle.h"


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


class BASE_API UContextBase : public std::enable_shared_from_this<UContextBase> {

#pragma region Schedule Wrapper
    class BASE_API ISchedule_Interface {

    public:
        ISchedule_Interface() = default;
        virtual ~ISchedule_Interface() = default;

        DISABLE_COPY_MOVE(ISchedule_Interface)
        virtual void Execute(IServiceBase *pService) = 0;
    };

    class BASE_API UPackageNode final : public ISchedule_Interface {

        shared_ptr<IPackage_Interface> mPackage;

    public:
        void SetPackage(const shared_ptr<IPackage_Interface> &pkg);
        void Execute(IServiceBase *pService) override;
    };


    class BASE_API UTaskNode final : public ISchedule_Interface {

        std::function<void(IServiceBase *)> mTask;

    public:
        void SetTask(const std::function<void(IServiceBase *)> &task);
        void Execute(IServiceBase *pService) override;
    };


    class BASE_API UEventNode final : public ISchedule_Interface {

        shared_ptr<IEventParam_Interface> mEvent;

    public:
        void SetEventParam(const shared_ptr<IEventParam_Interface> &event);
        void Execute(IServiceBase *pService) override;
    };

    class BASE_API UTickerNode final : public ISchedule_Interface {

        ASteadyTimePoint mTickTime;
        ASteadyDuration mDeltaTime;

    public:
        UTickerNode();

        void SetCurrentTickTime(ASteadyTimePoint timepoint);
        void SetDeltaTime(ASteadyDuration delta);

        void Execute(IServiceBase *pService) override;
    };
#pragma endregion

    using AContextChannel = TConcurrentChannel<void(std::error_code, std::unique_ptr<ISchedule_Interface>)>;

public:
    UContextBase();
    virtual ~UContextBase();

    DISABLE_COPY_MOVE(UContextBase)

    void SetUpModule(IModuleBase *pModule);
    void SetUpServiceID(int64_t sid);
    void SetUpLibrary(const FSharedLibrary &library);

    [[nodiscard]] std::string GetServiceName() const;
    [[nodiscard]] int64_t GetServiceID() const;

    [[nodiscard]] UServer *GetServer() const;
    [[nodiscard]] IModuleBase *GetOwnerModule() const;

    [[nodiscard]] FContextHandle GenerateHandle();

    virtual bool Initial(const IDataAsset_Interface *pData);
    virtual awaitable<bool> AsyncInitial(const IDataAsset_Interface *pData);

    virtual int Shutdown(bool bForce, int second, const std::function<void(UContextBase *)> &func);
    int ForceShutdown();

    bool BootService();

    [[nodiscard]] shared_ptr<IPackage_Interface> BuildPackage() const;

    [[nodiscard]] IServiceBase *GetOwningService() const;
    [[nodiscard]] EContextState GetState() const;

    void PushPackage(const shared_ptr<IPackage_Interface> &pkg);
    void PushTask(const std::function<void(IServiceBase *)> &task);
    void PushEvent(const shared_ptr<IEventParam_Interface> &event);
    void PushTicker(ASteadyTimePoint timepoint, ASteadyDuration delta);

#pragma region Timer
    int64_t CreateTimer(const std::function<void(IServiceBase *)> &task, int delay, int rate = -1);
    void CancelTimer(int64_t tid) const;
#pragma endregion

#pragma region Event
    void ListenEvent(int event);
    void RemoveListener(int event);
    void DispatchEvent(const std::shared_ptr<IEventParam_Interface> &param) const;
#pragma endregion

private:
    awaitable<void> ProcessChannel();

private:
    IModuleBase *mModule;

    int64_t mServiceID;
    IServiceBase *mService;

    shared_ptr<IRecyclerBase> mPackagePool;
    unique_ptr<AContextChannel> mChannel;

    /** Loaded Library With Creator And Destroyer Of Service */
    FSharedLibrary mLibrary;

    unique_ptr<ASteadyTimer> mShutdownTimer;
    std::function<void(UContextBase *)> mShutdownCallback;

protected:
    std::atomic<EContextState> mState;
};
