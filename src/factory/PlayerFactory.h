#pragma once

#include "Common.h"

#include <memory>


class UAgent;
class IAgentHandler;
class IPlayerBase;

using std::unique_ptr;


class BASE_API IPlayerFactory_Interface {

public:
    virtual ~IPlayerFactory_Interface() = default;

    virtual unique_ptr<IPlayerBase> CreatePlayer() = 0;
    virtual unique_ptr<IAgentHandler> CreateAgentHandler() = 0;
};