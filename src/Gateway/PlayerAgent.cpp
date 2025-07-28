#include "PlayerAgent.h"
#include "Gateway.h"
#include "Server.h"
#include "Package.h"
#include "Timer/TimerModule.h"
#include "Monitor/Monitor.h"
#include "Network/Network.h"
#include "Event/EventModule.h"
#include "Monitor/Watchdog.h"

#include <spdlog/spdlog.h>


UAgentContext::UAgentContext()
    : mPlayerID(0),
      mConnectionID(0) {
}

int32_t UAgentContext::GetServiceID() const {
    return PLAYER_AGENT_ID;
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

void UAgentContext::OnHeartBeat(const std::shared_ptr<IPackageInterface> &pkg) const {
    if (mState != EContextState::IDLE && mState != EContextState::RUNNING)
        return;

    if (pkg == nullptr)
        return;

    SPDLOG_INFO("{:<20} - Heartbeat From Player[{}]", __FUNCTION__, mPlayerID);
    dynamic_cast<IPlayerAgent *>(GetService())->OnHeartBeat(pkg);
}

UGateway *UAgentContext::GetGateway() const {
    return dynamic_cast<UGateway *>(GetOwner());
}

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

void IPlayerAgent::SendToPlayer(const int64_t pid, const std::shared_ptr<IPackageInterface> &pkg) const {
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

void IPlayerAgent::SendToClient(const std::shared_ptr<IPackageInterface> &pkg) const {
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

FTimerHandle IPlayerAgent::SetSteadyTimer(const std::function<void(IServiceBase *)> &task, const int delay, const int rate) const {
    if (auto *timer = GetModule<UTimerModule>()) {
        return timer->SetSteadyTimer(PLAYER_AGENT_ID, GetPlayerID(), task, delay, rate);
    }
    return { -1, true };
}

FTimerHandle IPlayerAgent::SetSystemTimer(const std::function<void(IServiceBase *)> &task, int delay, int rate) const {
    if (auto *timer = GetModule<UTimerModule>()) {
        return timer->SetSystemTimer(PLAYER_AGENT_ID, GetPlayerID(), task, delay, rate);
    }
    return { -1, false };
}

void IPlayerAgent::CancelTimer(const FTimerHandle &handle) {
    auto *timerModule = GetModule<UTimerModule>();
    if (timerModule == nullptr)
        return;

    if (handle.id > 0) {
        timerModule->CancelTimer(handle);
    } else {
        timerModule->CancelPlayerTimer(GetPlayerID());
    }
}

void IPlayerAgent::OnHeartBeat(const std::shared_ptr<IPackageInterface> &pkg) {
}

void IPlayerAgent::ListenEvent(const int event) const {
    if (auto *eventModule = GetModule<UEventModule>()) {
        eventModule->ListenEvent(event, PLAYER_AGENT_ID, GetPlayerID());
    }
}

void IPlayerAgent::RemoveListener(const int event) const {
    if (auto *eventModule = GetModule<UEventModule>()) {
        eventModule->RemoveListener(event, PLAYER_AGENT_ID, GetPlayerID());
    }
}

void IPlayerAgent::SendToClient(const int64_t, const std::shared_ptr<IPackageInterface> &) const {
    // Hide This Function In Player Agent, Do Nothing
}
