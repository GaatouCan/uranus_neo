#pragma once

#include "Common.h"

#include <memory>


class UAgent;
class IAgentHandler;
class IPlayerBase;

using std::unique_ptr;


class BASE_API IPlayerFactory_Interface {

public:
    IPlayerFactory_Interface() = default;
    virtual ~IPlayerFactory_Interface() = default;

    DISABLE_COPY_MOVE(IPlayerFactory_Interface)

    virtual void Initial() = 0;

    virtual unique_ptr<IPlayerBase> CreatePlayer() const = 0;
    virtual unique_ptr<IAgentHandler> CreateAgentHandler() const = 0;
};