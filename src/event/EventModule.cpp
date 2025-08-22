#include "EventModule.h"
#include "Context.h"
#include "Agent.h"
#include "Server.h"

#include <ranges>


UEventModule::UEventModule()
    : bServiceWaiting(false),
      bPlayerWaiting(false) {
}

void UEventModule::Dispatch(const std::shared_ptr<IEventParam_Interface> &event) {
    if (mState != EModuleState::RUNNING || event == nullptr)
        return;

    if (event->GetEventType() < 0)
        return;

    {
        std::shared_lock lock(mServiceListenerMutex);
        if (const auto iter = mServiceListenerMap.find(event->GetEventType()); iter != mServiceListenerMap.end()) {
            for (const auto &weak : iter->second | std::views::values) {
                if (const auto context = weak.lock()) {
                    context->PushEvent(event);
                }
            }
        }
    }

    {
        std::shared_lock lock(mPlayerListenerMutex);
        if (const auto iter = mPlayerListenerMap.find(event->GetEventType()); iter != mPlayerListenerMap.end()) {
            for (const auto &weak : iter->second | std::views::values) {
                if (const auto agent = weak.lock()) {
                    agent->PushEvent(event);
                }
            }
        }
    }
}

void UEventModule::ServiceListenEvent(const int64_t sid, const weak_ptr<UContext> &weakPtr, const int event) {
    if (mState < EModuleState::INITIALIZED)
        return;

    if (sid < 0 || event < 0)
        return;

    std::unique_lock lock(mServiceListenerMutex);
    mServiceListenerMap[event].insert_or_assign(sid, weakPtr);
}

void UEventModule::RemoveServiceListenerByEvent(const int64_t sid, const int event) {
    if (mState < EModuleState::INITIALIZED || GetServer()->GetState() == EServerState::TERMINATED)
        return;

    if (sid < 0 || event < 0)
        return;

    std::unique_lock lock(mServiceListenerMutex);
    const auto iter = mServiceListenerMap.find(event);
    if (iter == mServiceListenerMap.end())
        return;

    iter->second.erase(sid);

    if (iter->second.empty())
        mServiceListenerMap.erase(iter);
}

void UEventModule::RemoveServiceListener(const int64_t sid) {
    if (mState < EModuleState::INITIALIZED || GetServer()->GetState() == EServerState::TERMINATED)
        return;

    if (sid < 0)
        return;

    std::unique_lock lock(mServiceListenerMutex);
    for (auto &val: mServiceListenerMap | std::views::values) {
        val.erase(sid);
    }
}

void UEventModule::PlayerListenEvent(const int64_t pid, const weak_ptr<UAgent> &weakPtr, const int event) {
    if (mState < EModuleState::INITIALIZED)
        return;

    if (pid <= 0 || event < 0)
        return;

    std::unique_lock lock(mPlayerListenerMutex);
    mPlayerListenerMap[event].insert_or_assign(pid, weakPtr);
}

void UEventModule::RemovePlayerListenerByEvent(const int64_t pid, const int event) {
    if (mState < EModuleState::INITIALIZED || GetServer()->GetState() == EServerState::TERMINATED)
        return;

    if (pid <= 0 || event < 0)
        return;

    std::unique_lock lock(mPlayerListenerMutex);
    const auto iter = mPlayerListenerMap.find(event);
    if (iter == mPlayerListenerMap.end())
        return;

    iter->second.erase(pid);

    if (iter->second.empty())
        mPlayerListenerMap.erase(iter);
}

void UEventModule::RemovePlayerListener(const int64_t pid) {
    if (mState < EModuleState::INITIALIZED || GetServer()->GetState() == EServerState::TERMINATED)
        return;

    if (pid <= 0)
        return;

    std::unique_lock lock(mPlayerListenerMutex);
    for (auto &val: mPlayerListenerMap | std::views::values) {
        val.erase(pid);
    }
}
