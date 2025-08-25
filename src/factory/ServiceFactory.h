#pragma once

#include "base/SharedLibrary.h"
#include "ServiceHandle.h"

class BASE_API IServiceFactory_Interface {

public:
    IServiceFactory_Interface() = default;
    virtual ~IServiceFactory_Interface() = default;

    DISABLE_COPY_MOVE(IServiceFactory_Interface)

    virtual void LoadService() = 0;

    [[nodiscard]] virtual FSharedLibrary FindService(const std::string &name) const = 0;

    [[nodiscard]] virtual FServiceHandle CreateInstance(const std::string &path) const = 0;
    virtual void DestroyInstance(IServiceBase *pService) const = 0;
};
