#include "PlayerAgent.h"
#include "AgentContext.h"
#include "Gateway.h"
#include "Server.h"
#include "Base/Package.h"
#include "Timer/TimerModule.h"
#include "Network/Network.h"
#include "Event/EventModule.h"

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

void IPlayerAgent::SendToPlayer(const int64_t pid, const std::shared_ptr<IPackage_Interface> &pkg) const {
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

void IPlayerAgent::SendToClient(const std::shared_ptr<IPackage_Interface> &pkg) const {
    if (mState != EServiceState::RUNNING)
        return;

    if (pkg == nullptr)
        return;

    if (const auto *network = GetModule<UNetwork>()) {
        SPDLOG_TRACE("{:<20} - Player[{}]", __FUNCTION__, GetPlayerID());

        pkg->SetSource(PLAYER_AGENT_ID);
        pkg->SetTarget(CLIENT_TARGET_ID);

        network->SendToClient(dynamic_cast<UAgentContext *>(mContext)->GetConnectionID(), pkg);
    }
}

//
// FTimerHandle IPlayerAgent::SetSteadyTimer(const std::function<void(IServiceBase *)> &task, const int delay, const int rate) const {
//     if (auto *module = GetModule<UTimerModule>()) {
//         return module->SetSteadyTimer(PLAYER_AGENT_ID, GetPlayerID(), task, delay, rate);
//     }
//     return { -1, true };
// }
//
// FTimerHandle IPlayerAgent::SetSystemTimer(const std::function<void(IServiceBase *)> &task, int delay, int rate) const {
//     if (auto *module = GetModule<UTimerModule>()) {
//         return module->SetSystemTimer(PLAYER_AGENT_ID, GetPlayerID(), task, delay, rate);
//     }
//     return { -1, false };
// }
//
// void IPlayerAgent::CancelTimer(const FTimerHandle &handle) {
//     if (auto *module = GetModule<UTimerModule>()) {
//         if (handle.id > 0) {
//             module->CancelTimer(handle);
//         } else {
//             module->CancelPlayerTimer(GetPlayerID());
//         }
//     }
// }

void IPlayerAgent::OnHeartBeat(const std::shared_ptr<IPackage_Interface> &pkg) {
}

void IPlayerAgent::ListenEvent(const int event) const {
    if (auto *module = GetModule<UEventModule>()) {
        module->ListenEvent(event, PLAYER_AGENT_ID, GetPlayerID());
    }
}

void IPlayerAgent::RemoveListener(const int event) const {
     if (auto *module = GetModule<UEventModule>()) {
        module->RemoveListener(event, PLAYER_AGENT_ID, GetPlayerID());
    }
}

void IPlayerAgent::SendToClient(const int64_t, const std::shared_ptr<IPackage_Interface> &) const {
    // Hide This Function In Player Agent, Do Nothing
}
