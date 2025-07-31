#include "AgentContext.h"
#include "Gateway.h"
#include "PlayerAgent.h"

#include <spdlog/spdlog.h>


UAgentContext::UAgentContext()
    : mPlayerID(0),
      mConnectionID(0) {
}

UAgentContext::~UAgentContext() {
}

void UAgentContext::SetPlayerID(const int64_t pid) {
    mPlayerID = pid;
}

void UAgentContext::SetConnectionID(const int64_t cid) {
    mConnectionID = cid;
}

int64_t UAgentContext::GetPlayerID() const {
    return mPlayerID;
}

int64_t UAgentContext::GetConnectionID() const {
    return mConnectionID;
}

int32_t UAgentContext::GetServiceID() const {
    return PLAYER_AGENT_ID;
}

UGateway *UAgentContext::GetGateway() const {
    return dynamic_cast<UGateway *>(GetOwnerModule());
}

std::shared_ptr<UAgentContext> UAgentContext::GetOtherAgent(const int64_t pid) const {
    if (mState < EContextState::INITIALIZED || mState >= EContextState::WAITING)
        return nullptr;

    if (pid <= 0 || pid == mPlayerID)
        return nullptr;

    if (const auto *gateway = GetGateway()) {
        return gateway->FindPlayerAgent(pid);
    }

    return nullptr;
}

void UAgentContext::OnHeartBeat(const std::shared_ptr<IPackage_Interface> &pkg) const {
    if (mState != EContextState::IDLE && mState != EContextState::RUNNING)
        return;

    if (pkg == nullptr)
        return;

    SPDLOG_TRACE("{:<20} - Heartbeat From Player[{}]", __FUNCTION__, mPlayerID);
    dynamic_cast<IPlayerAgent *>(GetOwningService())->OnHeartBeat(pkg);
}
