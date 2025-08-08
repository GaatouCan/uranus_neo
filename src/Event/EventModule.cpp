#include "EventModule.h"
#include "Server.h"
#include "ContextBase.h"
#include "Utils.h"

#include <ranges>


UEventModule::UEventModule() {
}

void UEventModule::Dispatch(const std::shared_ptr<IEventParam_Interface> &event) {
    if (mState != EModuleState::RUNNING || event == nullptr)
        return;

    if (event->GetEventType() < 0)
        return;

    std::unique_lock lock(mListenerMutex);
    const auto iter = mListenerMap.find(event->GetEventType());
    if (iter == mListenerMap.end())
        return;

    for (auto it = iter->second.begin(); it != iter->second.end();) {
        if (const auto ctx = it->lock()) {
            ctx->PushEvent(event);
            ++it;
            continue;
        }

        it = iter->second.erase(it);
    }
}

void UEventModule::ListenEvent(const std::weak_ptr<UContextBase> &wPtr, const int event) {
    if (mState < EModuleState::INITIALIZED)
        return;

    if (wPtr.expired() || event < 0)
        return;

    std::unique_lock lock(mListenerMutex);
    utils::CleanUpWeakPointerSet(mListenerMap[event]);
    mListenerMap[event].emplace(wPtr);
}

void UEventModule::RemoveListenEvent(const std::weak_ptr<UContextBase> &wPtr, const int event) {
    if (mState < EModuleState::INITIALIZED)
        return;

    if (wPtr.expired() || event < 0)
        return;

    std::unique_lock lock(mListenerMutex);
    utils::CleanUpWeakPointerSet(mListenerMap[event]);
    mListenerMap[event].erase(wPtr);

    if (mListenerMap[event].empty())
        mListenerMap.erase(event);
}

void UEventModule::RemoveListener(const std::weak_ptr<UContextBase> &wPtr) {
    if (wPtr.expired() || mState < EModuleState::INITIALIZED)
        return;

    std::unique_lock lock(mListenerMutex);
    for (auto &val: mListenerMap | std::views::values) {
        utils::CleanUpWeakPointerSet(val);
        val.erase(wPtr);
    }
}
