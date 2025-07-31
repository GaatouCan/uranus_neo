#pragma once

#include "Common.h"

class BASE_API ILogicConfig_Interface {

public:
    ILogicConfig_Interface() = default;
    virtual ~ILogicConfig_Interface() = default;

    DISABLE_COPY_MOVE(ILogicConfig_Interface)

    virtual bool Initial() = 0;
};
