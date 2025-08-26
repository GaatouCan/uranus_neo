#pragma once

#include "ServiceHandle.h"

class BASE_API IServiceFactory_Interface {

public:
    IServiceFactory_Interface() = default;
    virtual ~IServiceFactory_Interface() = default;

    DISABLE_COPY_MOVE(IServiceFactory_Interface)

    virtual void LoadService() = 0;

    [[nodiscard]] virtual FServiceHandle CreateInstance(const std::string &path) = 0;
    virtual void DestroyInstance(IServiceBase *pService, const std::string &path) = 0;
};
