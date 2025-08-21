#pragma once

#include "Common.h"

#include <memory>

class IPlayerBase;
class UAgent;
class IAgentHandler;

using std::unique_ptr;

class BASE_API IPlayerFactory_Interface {

public:
    virtual ~IPlayerFactory_Interface() = default;

    virtual unique_ptr<IPlayerBase> CreatePlayer() = 0;
    virtual unique_ptr<IAgentHandler> CreateAgentHandler() = 0;
};