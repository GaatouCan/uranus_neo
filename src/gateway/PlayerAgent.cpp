#include "PlayerAgent.h"
#include "AgentContext.h"
#include "Gateway.h"
#include "Server.h"
#include "base/Package.h"
#include "timer/TimerModule.h"
#include "network/Network.h"

#include <spdlog/spdlog.h>


IPlayerAgent::IPlayerAgent() {

}

IPlayerAgent::~IPlayerAgent() {

}

std::string IPlayerAgent::GetServiceName() const {
    if (GetPlayerID() < 0) {
        return "Player Agent Unknown";
    }
    return std::format("Player Agent - {}", GetPlayerID());
}

int64_t IPlayerAgent::GetPlayerID() const {
    if (mContext == nullptr)
        return 0;
    return dynamic_cast<UAgentContext *>(mContext)->GetPlayerID();
}

void IPlayerAgent::SendToPlayer(const int64_t pid, const FPackageHandle &pkg) const {
    if (mState != EServiceState::RUNNING)
        return;

    // Do Not Send To Self
    if (pkg == nullptr || pid == GetPlayerID())
        return;

    if (const auto *gateway = GetModule<UGateway>()) {
        SPDLOG_TRACE("{:<20} - From Player[{}] To Player[{}]", __FUNCTION__, GetPlayerID(), pid);

        pkg->SetSource(PLAYER_AGENT_ID);
        pkg->SetTarget(PLAYER_AGENT_ID);

        gateway->SendToPlayer(pid, pkg);
    }
}

void IPlayerAgent::PostToPlayer(const int64_t pid, const std::function<void(IServiceBase *)> &task) const {
    if (mState != EServiceState::RUNNING)
        return;

    // Do Not Send To Self
    if (task == nullptr || pid == GetPlayerID())
        return;

    if (const auto *gateway = GetModule<UGateway>()) {
        SPDLOG_TRACE("{:<20} - From Player[{}] To Player[{}]", __FUNCTION__, GetPlayerID(), pid);
        gateway->PostToPlayer(pid, task);
    }
}

void IPlayerAgent::SendToClient(const FPackageHandle &pkg) const {
    if (mState != EServiceState::RUNNING)
        return;

    if (pkg == nullptr)
        return;

    if (const auto *network = GetModule<UNetwork>()) {
        SPDLOG_TRACE("{:<20} - Player[{}]", __FUNCTION__, GetPlayerID());

        pkg->SetSource(PLAYER_AGENT_ID);
        pkg->SetTarget(CLIENT_TARGET_ID);

        network->SendToClient(dynamic_cast<UAgentContext *>(mContext)->GetConnectionKey(), pkg);
    }
}

void IPlayerAgent::OnHeartBeat(const FPackageHandle &pkg) {
}

void IPlayerAgent::SendToClient(const int64_t, const FPackageHandle &) const {
    // Hide This Function In Player Agent, Do Nothing
}
