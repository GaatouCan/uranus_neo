#pragma once

#include "PlayerHandle.h"

#include <memory>


class UAgent;
class IAgentWorker;
class IPlayerBase;


class BASE_API IPlayerFactory_Interface {

public:
    IPlayerFactory_Interface() = default;
    virtual ~IPlayerFactory_Interface() = default;

    DISABLE_COPY_MOVE(IPlayerFactory_Interface)

    virtual void Initial() = 0;

    [[nodiscard]] virtual FPlayerHandle CreatePlayer() const = 0;
    [[nodiscard]] virtual unique_ptr<IAgentWorker> CreateAgentWorker() const = 0;

    virtual void DestroyPlayer(IPlayerBase *) const = 0;
};