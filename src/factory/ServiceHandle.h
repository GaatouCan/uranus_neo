#pragma once

#include "base/SharedLibrary.h"


class IServiceBase;
class IServiceFactory_Interface;


class BASE_API FServiceHandle final {

    IServiceBase *mService;
    FSharedLibrary mLibrary;

public:
    FServiceHandle();

    explicit FServiceHandle(const FSharedLibrary &library);
    ~FServiceHandle();

    DISABLE_COPY(FServiceHandle)

    FServiceHandle(FServiceHandle &&rhs) noexcept;
    FServiceHandle &operator=(FServiceHandle &&rhs) noexcept;

    [[nodiscard]] bool IsValid() const;

    IServiceBase *operator->() const noexcept;
    IServiceBase &operator*() const noexcept;

    [[nodiscard]] IServiceBase *Get() const noexcept;

    bool operator==(const FServiceHandle &rhs) const noexcept;
    bool operator==(nullptr_t) const noexcept;

    explicit operator bool() const noexcept;

    void Release();
};
