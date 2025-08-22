#include "AgentWorker.h"
#include "Agent.h"

#include <format>
#include <stdexcept>


IAgentWorker::IAgentWorker()
    : mAgent(nullptr) {
}

IAgentWorker::~IAgentWorker() {
}

void IAgentWorker::SetUpAgent(UAgent *agent) {
    mAgent = agent;
}

UAgent *IAgentWorker::GetAgent() const {
    return mAgent;
}

UServer *IAgentWorker::GetServer() const {
    if (!mAgent)
        throw std::logic_error(std::format("{} - Agent not set", __FUNCTION__));

    return mAgent->GetServer();
}
