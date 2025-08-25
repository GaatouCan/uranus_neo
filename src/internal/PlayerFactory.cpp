#include "PlayerFactory.h"

#include <spdlog/spdlog.h>


UPlayerFactory::UPlayerFactory()
    : mCreator(nullptr),
      mDestroyer(nullptr) {
}

UPlayerFactory::~UPlayerFactory() {
}

void UPlayerFactory::Initial() {
    std::string agent = PLAYER_AGENT_DIRECTORY;

#if defined(_WIN32) || defined(_WIN64)
    agent += "/agent.dll";
#else
    agent += "/libagent.so";
#endif

    if (mCreateAgentHandler == nullptr) {
        SPDLOG_ERROR("AgentHandler Creator Is Null");
        exit(-1);
    }

    mLibrary = FSharedLibrary(agent);

    if (!mLibrary.IsValid()) {
        SPDLOG_CRITICAL("{} - Failed To Load Player Library", __FUNCTION__);
        exit(-1);
    }

    mCreator = mLibrary.GetSymbol<APlayerCreator>("CreatePlayer");
    if (!mCreator) {
        SPDLOG_CRITICAL("{} - Failed To Load Player Creator", __FUNCTION__);
        exit(-2);
    }

    mDestroyer = mLibrary.GetSymbol<APlayerDestroyer>("DestroyPlayer");
    if (!mDestroyer) {
        SPDLOG_CRITICAL("{} - Failed To Load Player Destroyer", __FUNCTION__);
        exit(-3);
    }

    SPDLOG_INFO("{} - Successfully Loaded Agent Library", __FUNCTION__);
}

FPlayerHandle UPlayerFactory::CreatePlayer() {
    auto *pPlayer = std::invoke(mCreator);
    return FPlayerHandle{ pPlayer, this };
}

unique_ptr<IAgentHandler> UPlayerFactory::CreateAgentHandler() const {
    return std::invoke(mCreateAgentHandler);
}

void UPlayerFactory::DestroyPlayer(IPlayerBase *pPlayer) const {
    std::invoke(mDestroyer, pPlayer);
}
