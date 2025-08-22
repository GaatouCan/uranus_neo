#include "EventModule.h"
#include "Context.h"
#include "Agent.h"
#include "Server.h"

#include <ranges>


UEventModule::UEventModule() {
}

void UEventModule::Dispatch(const std::shared_ptr<IEventParam_Interface> &event) {
    if (mState != EModuleState::RUNNING || event == nullptr)
        return;

    if (event->GetEventType() < 0)
        return;

    {
        std::unique_lock lock(mServiceListenerMutex);
        const auto iter = mServiceListenerMap.find(event->GetEventType());
        if (iter == mServiceListenerMap.end())
            return;

        for (auto it = iter->second.begin(); it != iter->second.end();) {
            if (it->IsValid()) {
                it->Get()->PushEvent(event);
                ++it;
                continue;
            }

            // If The Handle Expired, Erase It
            iter->second.erase(it++);
        }
    }

    {
        std::shared_lock lock(mPlayerListenerMutex);
        const auto iter = mPlayerListenerMap.find(event->GetEventType());
        if (iter != mPlayerListenerMap.end()) {
            // for (const auto &pid : iter->second) {
            //     players.push_back(pid);
            //     del.insert(pid);
            // }
            for (auto it = iter->second.begin(); it != iter->second.end();) {
                if (it->IsValid()) {
                    it->Get()->PushEvent(event);
                    ++it;
                    continue;
                }

                iter->second.erase(it++);
            }
        }
    }
}

void UEventModule::ServiceListenEvent(const FContextHandle &handle, const int event) {
    if (mState < EModuleState::INITIALIZED)
        return;

    if (!handle.IsValid() || event < 0)
        return;

    std::unique_lock lock(mServiceListenerMutex);
    mServiceListenerMap[event].emplace(handle);
}

void UEventModule::RemoveServiceListenerByEvent(const FContextHandle &handle, const int event) {
    if (mState < EModuleState::INITIALIZED || GetServer()->GetState() == EServerState::TERMINATED)
        return;

    if (handle < 0 || event < 0)
        return;

    std::unique_lock lock(mServiceListenerMutex);
    const auto iter = mServiceListenerMap.find(event);
    if (iter == mServiceListenerMap.end())
        return;

    iter->second.erase(handle);

    if (iter->second.empty())
        mServiceListenerMap.erase(iter);
}

void UEventModule::RemoveServiceListener(const FContextHandle &handle) {
    if (mState < EModuleState::INITIALIZED || GetServer()->GetState() == EServerState::TERMINATED)
        return;

    if (handle < 0)
        return;

    std::unique_lock lock(mServiceListenerMutex);
    for (auto &val: mServiceListenerMap | std::views::values) {
        val.erase(handle);
    }
}

void UEventModule::PlayerListenEvent(const FAgentHandle &handle, int event) {
    if (mState < EModuleState::INITIALIZED)
        return;

    if (handle <= 0 || event < 0)
        return;

    std::unique_lock lock(mPlayerListenerMutex);
    mPlayerListenerMap[event].emplace(handle);
}

void UEventModule::RemovePlayerListenerByEvent(const FAgentHandle &handle, int event) {
    if (mState < EModuleState::INITIALIZED || GetServer()->GetState() == EServerState::TERMINATED)
        return;

    if (handle <= 0 || event < 0)
        return;

    std::unique_lock lock(mPlayerListenerMutex);
    const auto iter = mPlayerListenerMap.find(event);
    if (iter == mPlayerListenerMap.end())
        return;

    iter->second.erase(handle);

    if (iter->second.empty())
        mPlayerListenerMap.erase(iter);
}

void UEventModule::RemovePlayerListener(const FAgentHandle &handle) {
    if (mState < EModuleState::INITIALIZED || GetServer()->GetState() == EServerState::TERMINATED)
        return;

    if (handle <= 0)
        return;

    std::unique_lock lock(mPlayerListenerMutex);
    for (auto &val: mPlayerListenerMap | std::views::values) {
        val.erase(handle);
    }
}
