#pragma once

#include "Common.h"

#include <string>

class BASE_API IServiceBase {

public:
    IServiceBase();
    virtual ~IServiceBase();

    DISABLE_COPY_MOVE(IServiceBase)

    virtual std::string GetServiceName() const = 0;
};

