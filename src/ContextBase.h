#pragma once

#include "Base/Types.h"
#include "Base/SharedLibrary.h"

#include <memory>


class IServiceBase;
class IModuleBase;
class IRecyclerBase;
class FPackage;


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


class BASE_API IContextBase {

    class BASE_API INodeBase {

    protected:
        IServiceBase *const mService;

    public:
        INodeBase() = delete;

        explicit INodeBase(IServiceBase *service);
        virtual ~INodeBase() = default;

        DISABLE_COPY_MOVE(INodeBase)

        [[nodiscard]] IServiceBase *GetService() const;

        virtual void Execute() = 0;
    };

    using AContextChannel = TConcurrentChannel<void(std::error_code, std::shared_ptr<INodeBase>)>;

public:
    IContextBase();
    virtual ~IContextBase();

    DISABLE_COPY_MOVE(IContextBase)

private:
    IModuleBase *mModule;
    IServiceBase *mService;

    FSharedLibrary mLibrary;

    std::shared_ptr<IRecyclerBase> mPackagePool;
};
