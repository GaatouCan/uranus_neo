#include "AgentHandler.h"
#include "../Agent.h"

#include <format>
#include <stdexcept>


IAgentHandler::IAgentHandler()
    : mAgent(nullptr) {
}

IAgentHandler::~IAgentHandler() {
}

void IAgentHandler::SetUpAgent(UAgent *agent) {
    mAgent = agent;
}

UAgent *IAgentHandler::GetAgent() const {
    return mAgent;
}

UServer *IAgentHandler::GetServer() const {
    if (!mAgent)
        throw std::logic_error(std::format("{} - Agent not set", __FUNCTION__));

    return mAgent->GetServer();
}
