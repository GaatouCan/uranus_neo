#pragma once

#include "Common.h"
#include <memory>

class BASE_API IPlayerBase : public std::enable_shared_from_this<IPlayerBase> {

public:
    IPlayerBase();
    virtual ~IPlayerBase();

};
