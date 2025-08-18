#pragma once

#include "Common.h"

#include <concepts>


class BASE_API IEventParam_Interface {

public:
    virtual ~IEventParam_Interface() = default;

    [[nodiscard]] virtual int GetEventType() const = 0;
};

template <typename T>
concept CEventType = std::derived_from<T, IEventParam_Interface>;
