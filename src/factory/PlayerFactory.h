#pragma once

#include "PlayerHandle.h"

#include <memory>

class UPlayerAgent;
class IAgentHandler;
class IPlayerBase;

using std::unique_ptr;
using std::shared_ptr;
using std::make_unique;
using std::make_shared;


class BASE_API IPlayerFactory_Interface {

public:
    IPlayerFactory_Interface() = default;
    virtual ~IPlayerFactory_Interface() = default;

    DISABLE_COPY_MOVE(IPlayerFactory_Interface)

    virtual void Initial() = 0;

    [[nodiscard]] virtual FPlayerHandle CreatePlayer() = 0;
    [[nodiscard]] virtual unique_ptr<IAgentHandler> CreateAgentHandler() const = 0;

    virtual void DestroyPlayer(IPlayerBase *) = 0;
};