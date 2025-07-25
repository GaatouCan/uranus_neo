#pragma once

#include "Common.h"

class BASE_API IEventParam_Interface {

public:
    virtual ~IEventParam_Interface() = default;

    [[nodiscard]] virtual int GetEventType() const = 0;
};
