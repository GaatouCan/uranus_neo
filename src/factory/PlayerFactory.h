#pragma once

#include "PlayerHandle.h"

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

    [[nodiscard]] virtual FPlayerHandle CreatePlayer() = 0;
    [[nodiscard]] virtual unique_ptr<IAgentHandler> CreateAgentHandler() const = 0;

    virtual void DestroyPlayer(IPlayerBase *) const = 0;
};