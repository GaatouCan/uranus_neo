#pragma once

#include "Common.h"

#include <cstdint>

class IPackage_Interface;

class BASE_API IPlayerBase {

public:
    IPlayerBase();
    virtual ~IPlayerBase();

    [[nodiscard]] int64_t GetPlayerID() const;

    void OnPackage(IPackage_Interface *pkg) const;
};

