#include "RouteModule.h"
#include "Server.h"
#include "AgentBase.h"
#include "base/Package.h"
#include "service/ServiceModule.h"
#include "service/ServiceAgent.h"
#include "gateway/Gateway.h"
#include "gateway/PlayerAgent.h"


URouteModule::URouteModule() {
}

URouteModule::~URouteModule() {
}

void URouteModule::PostPackage(const FPackageHandle &pkg, const int64_t pid) const {
    if (mState != EModuleState::RUNNING)
        return;

    if (pkg == nullptr)
        return;

    int64_t target = pkg->GetTarget();

    if (target > 0) {
        const auto *serviceModule = GetServer()->GetModule<UServiceModule>();
        if (!serviceModule)
            return;

        if (const auto agent = serviceModule->FindService(target)) {
            agent->PushPackage(pkg);
        }

        return;
    }

    if (target == PLAYER_TARGET_ID) {
        target = pid;
    }

    if (target < 0)
        return;

    const auto *gateway = GetServer()->GetModule<UGateway>();
    if (!gateway)
        return;

    if (const auto agent = gateway->FindPlayer(target)) {
        agent->PushPackage(pkg);
    }
}

void URouteModule::PostPackage(const std::string &name, const FPackageHandle &pkg) const {
    if (mState != EModuleState::RUNNING)
        return;

    if (name.empty() || pkg == nullptr)
        return;

    const auto *serviceModule = GetServer()->GetModule<UServiceModule>();
    if (!serviceModule)
        return;

    if (const auto agent = serviceModule->FindService(name)) {
        pkg->SetTarget(agent->GetServiceID());
        agent->PushPackage(pkg);
    }
}

void URouteModule::PostTask(const bool bToService, const int64_t target, const AActorTask &task) const {
    if (mState != EModuleState::RUNNING)
        return;

    if (target < 0 || task == nullptr)
        return;

    if (bToService) {
        const auto *serviceModule = GetServer()->GetModule<UServiceModule>();
        if (!serviceModule)
            return;

        if (const auto agent = serviceModule->FindService(target)) {
            agent->PushTask(task);
        }
    } else {
        const auto *gateway = GetServer()->GetModule<UGateway>();
        if (!gateway)
            return;

        if (const auto agent = gateway->FindPlayer(target)) {
            agent->PushTask(task);
        }
    }
}

void URouteModule::PostTask(const std::string &name, const AActorTask &task) const {
    if (mState != EModuleState::RUNNING)
        return;

    if (name.empty() || task == nullptr)
        return;

    const auto *serviceModule = GetServer()->GetModule<UServiceModule>();
    if (!serviceModule)
        return;

    if (const auto agent = serviceModule->FindService(name)) {
        agent->PushTask(task);
    }
}

void URouteModule::SendToClient(int64_t pid, const FPackageHandle &pkg) const {
    if (mState != EModuleState::RUNNING)
        return;

    if (pid <= 0 || pkg == nullptr)
        return;

    auto *gateway = GetServer()->GetModule<UGateway>();
    if (!gateway)
        return;

    if (const auto agent = gateway->FindPlayer(pid)) {
        pkg->SetTarget(CLIENT_TARGET_ID);
        agent->SendPackage(pkg);
    }
}
