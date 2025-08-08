#include "EventModule.h"
#include "ContextBase.h"
#include "Server.h"

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
        if (it->IsValid()) {
            it->Get()->PushEvent(event);
            ++it;
            continue;
        }

        // If The Handle Expired, Erase It
        it = iter->second.erase(it);
    }
}

void UEventModule::ListenEvent(const FContextHandle &handle, const int event) {
    if (mState < EModuleState::INITIALIZED)
        return;

    if (!handle.IsValid() || event < 0)
        return;

    std::unique_lock lock(mListenerMutex);
    mListenerMap[event].emplace(handle);
}

void UEventModule::RemoveListenEvent(const FContextHandle &handle, const int event) {
    if (mState < EModuleState::INITIALIZED)
        return;

    if (handle < 0 || event < 0)
        return;

    std::unique_lock lock(mListenerMutex);
    const auto iter = mListenerMap.find(event);
    if (iter == mListenerMap.end())
        return;

    iter->second.erase(handle);

    if (iter->second.empty())
        mListenerMap.erase(iter);
}

void UEventModule::RemoveListener(const FContextHandle &handle) {
    if (mState < EModuleState::INITIALIZED)
        return;

    if (handle < 0)
        return;

    std::unique_lock lock(mListenerMutex);
    for (auto &val: mListenerMap | std::views::values) {
        val.erase(handle);
    }
}
