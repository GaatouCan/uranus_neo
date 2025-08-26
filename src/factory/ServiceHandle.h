#pragma once

#include "Common.h"

#include <string>


class IServiceBase;
class IServiceFactory_Interface;


class BASE_API FServiceHandle final {

    IServiceBase *mService;
    IServiceFactory_Interface *mFactory;

    std::string mPath;

public:
    FServiceHandle();

    FServiceHandle(IServiceBase *pService, IServiceFactory_Interface *pFactory, const std::string &path);
    ~FServiceHandle();

    DISABLE_COPY(FServiceHandle)

    FServiceHandle(FServiceHandle &&rhs) noexcept;
    FServiceHandle &operator=(FServiceHandle &&rhs) noexcept;

    const std::string &GetPath() const;

    [[nodiscard]] bool IsValid() const;

    IServiceBase *operator->() const noexcept;
    IServiceBase &operator*() const noexcept;

    [[nodiscard]] IServiceBase *Get() const noexcept;

    bool operator==(const FServiceHandle &rhs) const noexcept;
    bool operator==(nullptr_t) const noexcept;

    explicit operator bool() const noexcept;

    void Release();
};
