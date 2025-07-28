#pragma once

#include "Common.h"

#include <cstdint>

struct BASE_API FTimerHandle {
    int64_t id;
    bool bSteady;
};
