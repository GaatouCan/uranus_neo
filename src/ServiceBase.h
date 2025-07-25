#pragma once

#include "Common.h"

#include <string>
#include <memory>

class FPackage;
class IContextBase;
class IEventParam_Interface;
class IDataAsset_Interface;

enum class BASE_API EServiceState {
    CREATED,
    INITIALIZED,
    RUNNING,
    TERMINATED,
};

class BASE_API IServiceBase {

    friend class IContextBase;

protected:
    void SetUpContext(IContextBase *context);

public:
    IServiceBase();
    virtual ~IServiceBase();

    DISABLE_COPY_MOVE(IServiceBase)

    [[nodiscard]] virtual std::string GetServiceName() const = 0;


    virtual bool Initial(const IDataAsset_Interface *data);
    virtual bool Start();
    virtual void Stop();

    virtual void OnPackage(const std::shared_ptr<FPackage> &pkg);
    virtual void OnEvent(const std::shared_ptr<IEventParam_Interface> &event);
};

