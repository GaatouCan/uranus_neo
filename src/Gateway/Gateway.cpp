#include "Gateway.h"
#include "PlayerAgent.h"
#include "Server.h"
#include "Service/LibraryHandle.h"
#include "Package.h"
#include "Network/Network.h"
#include "Service/ServiceModule.h"

#include <spdlog/spdlog.h>


UGateway::UGateway()
    : mLibrary(nullptr) {
}

UGateway::~UGateway() {
    Stop();
}

void UGateway::OnPlayerLogin(const int64_t pid, const int64_t cid) {
    if (mState != EModuleState::RUNNING)
        return;

    const auto agent = make_shared<UAgentContext>();

    agent->SetUpModule(this);
    agent->SetUpHandle(mLibrary);
    agent->SetPlayerID(pid);
    agent->SetConnectionID(cid);

    {
        std::unique_lock lock(mMutex);

        mPlayerMap[pid] = agent;
        mConnToPlayer[cid] = pid;
    }

    agent->Initial(nullptr);
    agent->BootService();

    SPDLOG_INFO("{:<20} - Player[{}] Login", __FUNCTION__, pid);
}

void UGateway::OnPlayerLogout(const int64_t pid) {
    if (mState != EModuleState::RUNNING)
        return;

    shared_ptr<UAgentContext> agent;

    {
        std::unique_lock lock(mMutex);

        const auto iter = mPlayerMap.find(pid);
        if (iter == mPlayerMap.end())
            return;

        agent = iter->second;
        mPlayerMap.erase(iter);

        if (agent == nullptr)
            return;

        mConnToPlayer.erase(agent->GetConnectionID());
    }

    SPDLOG_INFO("{:<20} - Player[{}] Logout", __FUNCTION__, agent->GetPlayerID());
    agent->Shutdown(false, 0, nullptr);
}

int64_t UGateway::GetConnectionID(const int64_t pid) const {
    if (mState != EModuleState::RUNNING)
        return -1;

    std::shared_lock lock(mMutex);

    const auto iter = mPlayerMap.find(pid);
    if (iter == mPlayerMap.end())
        return 0;

    if (const auto agent = iter->second) {
        return agent->GetConnectionID();
    }

    return -2;
}

std::shared_ptr<UAgentContext> UGateway::FindPlayerAgent(const int64_t pid) const {
    if (mState != EModuleState::RUNNING)
        return nullptr;

    std::shared_lock lock(mMutex);
    const auto iter = mPlayerMap.find(pid);
    return iter == mPlayerMap.end() ? nullptr : iter->second;
}

void UGateway::Initial() {
    if (mState != EModuleState::CREATED)
        return;

    std::string agent = PLAYER_AGENT_DIRECTORY;

#if defined(_WIN32) || defined(_WIN64)
    agent += "/agent.dll";
#else
    agent += "/libagent.so";
#endif

    mLibrary = new FLibraryHandle();
    if (!mLibrary->LoadFrom(agent)) {
        SPDLOG_ERROR("Gateway Module Fail To Load Agent Library");
        GetServer()->Shutdown();
        exit(-5);
    }

    SPDLOG_INFO("Loaded Player Agent From {}", agent);
    mState = EModuleState::INITIALIZED;
}

void UGateway::Stop() {
    if (mState == EModuleState::STOPPED)
        return;

    mState = EModuleState::STOPPED;

    for (const auto &agent : mPlayerMap | std::views::values) {
        agent->Shutdown(false, 0, nullptr);
    }

    delete mLibrary;
}

void UGateway::SendToPlayer(const int64_t pid, const std::shared_ptr<IPackageInterface> &pkg) const {
    if (mState != EModuleState::RUNNING)
        return;

    if (pkg == nullptr)
        return;

    if (const auto agent = FindPlayerAgent(pid))
        agent->PushPackage(pkg);
}

void UGateway::PostToPlayer(const int64_t pid, const std::function<void(IServiceBase *)> &task) const {
    if (mState != EModuleState::RUNNING)
        return;

    if (task == nullptr)
        return;

    if (const auto agent = FindPlayerAgent(pid))
        agent->PushTask(task);
}

void UGateway::OnClientPackage(const int64_t pid, const std::shared_ptr<IPackageInterface> &pkg) const {
    if (mState != EModuleState::RUNNING)
        return;

    if (pkg == nullptr)
        return;

    // const auto source = pkg->GetSource();
    const auto target = pkg->GetTarget();

    // To Player Agent
    if (target == PLAYER_AGENT_ID) {
        SendToPlayer(pid, pkg);
        return;
    }

    if (const auto context = GetServer()->GetModule<UServiceModule>()->FindService(target))
        context->PushPackage(pkg);
}

void UGateway::SendToClient(const int64_t pid, const std::shared_ptr<IPackageInterface> &pkg) const {
    if (mState != EModuleState::RUNNING)
        return;

    if (pkg == nullptr || pid < 0)
        return;

    const auto agent = FindPlayerAgent(pid);
    if (agent == nullptr)
        return;

    if (const auto *network = GetServer()->GetModule<UNetwork>())
        network->SendToClient(agent->GetConnectionID(), pkg);
}

void UGateway::OnHeartBeat(const int64_t pid, const std::shared_ptr<IPackageInterface> &pkg) const {
    if (mState != EModuleState::RUNNING)
        return;

    if (pkg == nullptr)
        return;

    if (const auto agent = FindPlayerAgent(pid))
        agent->OnHeartBeat(pkg);
}
