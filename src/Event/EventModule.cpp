#include "EventModule.h"
#include "Server.h"
#include "Service/ServiceContext.h"
#include "Gateway/AgentContext.h"
#include "Gateway/Gateway.h"
#include "Gateway/PlayerAgent.h"
#include "Service/ServiceModule.h"

#include <spdlog/spdlog.h>


UEventModule::UEventModule() {
}

void UEventModule::Dispatch(const std::shared_ptr<IEventParam_Interface> &event) {
    if (mState != EModuleState::RUNNING)
        return;

    unordered_set<int32_t> serviceSet;
    unordered_set<int64_t> playerSet;

    unordered_set<int32_t> serviceDel;
    unordered_set<int64_t> playerDel;

    {
        std::shared_lock lock(mListenerMutex);
        if (const auto iter = mServiceListener.find(event->GetEventType()); iter != mServiceListener.end()) {
            serviceSet = iter->second;
        }
        if (const auto iter = mPlayerListener.find(event->GetEventType()); iter != mPlayerListener.end()) {
            playerSet = iter->second;
        }
    }

    const auto *service = GetServer()->GetModule<UServiceModule>();
    const auto *gateway = GetServer()->GetModule<UGateway>();

    if (service == nullptr || gateway == nullptr) {
        SPDLOG_ERROR("{:<20} - Service or Gateway Module Not Found", __FUNCTION__);
        return;
    }

    for (const auto &sid : serviceSet) {
        if (const auto context = service->FindService(sid)) {
            context->PushEvent(event);
        } else {
            serviceDel.insert(sid);
        }
    }

    for (const auto &player : playerSet) {
        if (const auto agent = gateway->FindPlayerAgent(player)) {
            agent->PushEvent(event);
        } else {
            playerDel.insert(player);
        }
    }

    if (serviceDel.empty() && playerDel.empty())
        return;

    // Erase Expired Services And Agents
    std::unique_lock lock(mListenerMutex);
    if (const auto iter = mServiceListener.find(event->GetEventType()); iter != mServiceListener.end()) {
        for (const auto &sid : serviceDel) {
            iter->second.erase(sid);
        }
    }
    if (const auto iter = mPlayerListener.find(event->GetEventType()); iter != mPlayerListener.end()) {
        for (const auto &sid : playerDel) {
            iter->second.erase(sid);
        }
    }
}

void UEventModule::ListenEvent(const int event, const int32_t sid, const int64_t pid) {
    if (mState != EModuleState::RUNNING)
        return;

    std::unique_lock lock(mListenerMutex);
    if (sid > 0) {
        mServiceListener[event].insert(sid);
    } else if (pid > 0) {
        mPlayerListener[event].insert(pid);
    }
}

void UEventModule::RemoveListener(const int event, const int32_t sid, const int64_t pid) {
    if (mState != EModuleState::RUNNING)
        return;

    std::unique_lock lock(mListenerMutex);
    if (sid > 0) {
        if (const auto iter = mServiceListener.find(event); iter != mServiceListener.end()) {
            iter->second.erase(sid);
        }
    } else if (pid > 0) {
        if (const auto iter = mPlayerListener.find(event); iter != mPlayerListener.end()) {
            iter->second.erase(pid);
        }
    }
}
