#include "AgentHandler.h"
#include "../gateway/PlayerAgent.h"

#include <format>
#include <stdexcept>


IAgentHandler::IAgentHandler()
    : mAgent(nullptr) {
}

IAgentHandler::~IAgentHandler() {
}

void IAgentHandler::SetUpAgent(UPlayerAgent *agent) {
    mAgent = agent;
}

UPlayerAgent *IAgentHandler::GetAgent() const {
    return mAgent;
}

UServer *IAgentHandler::GetServer() const {
    if (!mAgent)
        throw std::logic_error(std::format("{} - Agent not set", __FUNCTION__));

    return mAgent->GetServer();
}
