#pragma once

#include "factory/PlayerFactory.h"
#include "base/AgentHandler.h"
#include "base/SharedLibrary.h"

#include <functional>


class BASE_API UPlayerFactory final : public IPlayerFactory_Interface {

    using APlayerCreator = IPlayerBase *(*)();
    using APlayerDestroyer = void (*)(IPlayerBase *);

public:
    UPlayerFactory();
    ~UPlayerFactory() override;

    void Initial() override;

    template<class T, class... Args>
    requires std::derived_from<T, IAgentHandler>
    void SetAgentHandler(Args && ... args) {
        auto tuple = std::make_tuple(std::forward<Args>(args)...);

        mCreateAgentHandler = [tuple] {
            return std::apply([](auto && ... unpacked) {
                return std::make_unique<T>(unpacked...);
            }, tuple);
        };
    }

    [[nodiscard]] FPlayerHandle CreatePlayer() override;
    [[nodiscard]] unique_ptr<IAgentHandler> CreateAgentHandler() const override;

    void DestroyPlayer(IPlayerBase *pPlayer) const override;

private:
    FSharedLibrary mLibrary;

    APlayerCreator mCreator;
    APlayerDestroyer mDestroyer;

    std::function<unique_ptr<IAgentHandler>()> mCreateAgentHandler;
};
