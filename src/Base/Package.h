#pragma once

#include "Common.h"

#include <concepts>

class BASE_API IPackage_Interface {
    
public:
    IPackage_Interface() = default;
    virtual ~IPackage_Interface() = default;

    DISABLE_COPY_MOVE(IPackage_Interface)

    virtual void SetPackageID   (uint32_t   id)     = 0;
    virtual void SetSource      (int32_t    source) = 0;
    virtual void SetTarget      (int32_t    target) = 0;

    [[nodiscard]] virtual uint32_t  GetPackageID()  const = 0;
    [[nodiscard]] virtual int32_t   GetSource()     const = 0;
    [[nodiscard]] virtual int32_t   GetTarget()     const = 0;
};

template<class Type>
concept CPackageType = std::derived_from<Type, IPackage_Interface>;
