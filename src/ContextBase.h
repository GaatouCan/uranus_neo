#pragma once

#include "Base/Types.h"
#include "Base/SharedLibrary.h"

class IServiceBase;
class IModuleBase;
class IRecyclerBase;
class FPackage;
class UServer;


enum class BASE_API EContextState {
    CREATED,
    INITIALIZING,
    INITIALIZED,
    IDLE,
    RUNNING,
    WAITING,
    SHUTTING_DOWN,
    STOPPED
};


class BASE_API IContextBase : public std::enable_shared_from_this<IContextBase> {

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

    using AContextChannel = TConcurrentChannel<void(std::error_code, std::shared_ptr<INodeBase>)>;

public:
    IContextBase();
    virtual ~IContextBase();

    DISABLE_COPY_MOVE(IContextBase)

    void SetUpModule(IModuleBase *module);
    void SetUpLibrary(const FSharedLibrary &library);

    virtual bool Initial();

    virtual int Shutdown(bool bForce, int second, const std::function<void(IContextBase *)> &func);
    int ForceShutdown();

    bool BootService();

    [[nodiscard]] UServer *GetServer() const;
    [[nodiscard]] IModuleBase *GetOwnerModule() const;

    [[nodiscard]] shared_ptr<FPackage> BuildPackage() const;

private:
    void PushNode(const shared_ptr<INodeBase> &node);
    awaitable<void> ProcessChannel();

private:
    IModuleBase *mModule;

    IServiceBase *mService;

    FSharedLibrary mLibrary;

    shared_ptr<IRecyclerBase> mPackagePool;

    unique_ptr<AContextChannel> mChannel;

    unique_ptr<ASteadyTimer> mShutdownTimer;
    std::function<void(IContextBase *)> mShutdownCallback;

protected:
    std::atomic<EContextState> mState;
};
