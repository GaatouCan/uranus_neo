#include "Gateway.h"
#include "Server.h"
#include "Agent.h"
#include "network/Network.h"

#include <spdlog/spdlog.h>
#include <ranges>


UGateway::UGateway() {
}

void UGateway::Initial() {

}

void UGateway::Stop() {

}

UGateway::~UGateway() {
    Stop();
}

bool UGateway::IsAgentExist(const int64_t pid) const {
    if (mState != EModuleState::RUNNING)
        return false;

    std::shared_lock lock(mMutex);
    return mAgentMap.contains(pid);
}

shared_ptr<UAgent> UGateway::FindAgent(const int64_t pid) const {
    if (mState != EModuleState::RUNNING)
        return nullptr;

    std::shared_lock lock(mMutex);

    const auto iter = mAgentMap.find(pid);
    return iter == mAgentMap.end() ? nullptr : iter->second;
}

void UGateway::RemoveAgent(const int64_t pid) {
    if (mState != EModuleState::RUNNING)
        return;

    std::unique_lock lock(mMutex);
    mAgentMap.erase(pid);
}

void UGateway::OnPlayerLogin(const std::string &key, const int64_t pid) {
    if (mState != EModuleState::RUNNING)
        return;

    if (pid <= 0 || key.empty())
        return;

    const auto *network = GetServer()->GetModule<UNetwork>();
    if (!network)
        return;

    const auto conn = network->FindConnection(key);
    if (!conn)
        return;

    const auto agent = CreateAgent(pid);

    agent->SetUpModule(this);
    agent->SetUpConnection(conn);
    agent->SetUpPlayerID(pid);

    // TODO: Boot Agent
}

shared_ptr<UAgent> UGateway::CreateAgent(const int64_t pid) {
    if (mState != EModuleState::RUNNING)
        throw std::logic_error(std::format("{} - Called In Not RUNNING State", __FUNCTION__));

    shared_ptr<UAgent> agent = nullptr;

    bool bExist = false;
    {
        std::unique_lock lock(mMutex);
        if (const auto iter = mAgentMap.find(pid); iter != mAgentMap.end()) {
            bExist = true;
            agent = iter->second;
        } else {
            agent = make_shared<UAgent>();
            mAgentMap.insert_or_assign(pid, agent);
        }
    }

    if (bExist) {
        agent->OnRepeat();
    }

    return agent;
}
