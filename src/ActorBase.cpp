#include "ActorBase.h"
#include "AgentBase.h"

#include <spdlog/fmt/fmt.h>

IActorBase::IActorBase()
    : mAgent(nullptr) {
}

IActorBase::~IActorBase() {
}

void IActorBase::SetUpAgent(IAgentBase *agent) {
    mAgent = agent;
}

IAgentBase *IActorBase::GetAgent() const {
    if (!mAgent)
        throw std::logic_error(fmt::format("{} - Agent Has Not Set Up", __FUNCTION__));

    return mAgent;
}

asio::io_context &IActorBase::GetIOContext() const {
    if (!mAgent)
        throw std::logic_error(fmt::format("{} - Agent Has Not Set Up", __FUNCTION__));

    return mAgent->GetIOContext();
}

UServer *IActorBase::GetServer() const {
    if (!mAgent)
        throw std::logic_error(fmt::format("{} - Agent Has Not Set Up", __FUNCTION__));

    return mAgent->GetServer();
}

void IActorBase::OnPackage(IPackage_Interface *pkg) {
}

void IActorBase::OnEvent(IEventParam_Interface *event) {
}
