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
    class BASE_API INodeBase {

    protected:
        IServiceBase *const mService;

    public:
        INodeBase() = delete;

        explicit INodeBase(IServiceBase *service);
        virtual ~INodeBase() = default;

        DISABLE_COPY_MOVE(INodeBase)

        [[nodiscard]] IServiceBase *GetService() const;

        virtual void Execute();
    };

    /**
     * The Wrapper Of Package,
     * While Service Received Package
     */
    class BASE_API UPackageNode final : public INodeBase {

        shared_ptr<IPackage_Interface> mPackage;

    public:
        explicit UPackageNode(IServiceBase *service);
        ~UPackageNode() override = default;

        void SetPackage(const shared_ptr<IPackage_Interface> &pkg);
        void Execute() override;
    };

    /**
     * The Wrapper Of Task,
     * While The Service Received The Task
     */
    class BASE_API UTaskNode final : public INodeBase {

        std::function<void(IServiceBase *)> mTask;

    public:
        explicit UTaskNode(IServiceBase *service);
        ~UTaskNode() override = default;

        void SetTask(const std::function<void(IServiceBase *)> &task);
        void Execute() override;
    };

    /**
     * The Wrapper Of Event,
     * While The Service Received Event Parameter
     */
    class BASE_API UEventNode final : public INodeBase {

        shared_ptr<IEventParam_Interface> mEvent;

    public:
        explicit UEventNode(IServiceBase *service);
        ~UEventNode() override = default;

        void SetEventParam(const shared_ptr<IEventParam_Interface> &event);
        void Execute() override;
    };

    using AContextChannel = TConcurrentChannel<void(std::error_code, std::shared_ptr<INodeBase>)>;

public:
    IContextBase();
    virtual ~IContextBase();

    DISABLE_COPY_MOVE(IContextBase)

    void SetUpModule(IModuleBase *module);
    void SetUpLibrary(const FSharedLibrary &library);

    virtual bool Initial(const IDataAsset_Interface *data);
    virtual awaitable<bool> AsyncInitial(const IDataAsset_Interface *data);

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

private:
    void PushNode(const shared_ptr<INodeBase> &node);
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
