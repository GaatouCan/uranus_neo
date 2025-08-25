#pragma once

#include "Common.h"


class IServiceBase;
class IServiceFactory_Interface;


class BASE_API FServiceHandle final {

    IServiceBase *mService;
    IServiceFactory_Interface *mFactory;

public:
    FServiceHandle();

    FServiceHandle(IServiceBase *pService, IServiceFactory_Interface *pFactory);
    ~FServiceHandle();

    DISABLE_COPY(FServiceHandle)

    FServiceHandle(FServiceHandle &&rhs) noexcept;
    FServiceHandle &operator=(FServiceHandle &&rhs) noexcept;
};
