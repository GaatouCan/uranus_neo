#include "EventModule.h"
#include "AgentBase.h"
#include "Server.h"

#include <spdlog/spdlog.h>
#include <ranges>


void UEventModule::Initial() {
    if (mState != EModuleState::CREATED)
        throw std::logic_error(std::format("{} - Module[{}] Not In CREATED State", __FUNCTION__, GetModuleName()));

    mServiceTimer = std::make_unique<ASteadyTimer>(GetServer()->GetIOContext());
    mPlayerTimer = std::make_unique<ASteadyTimer>(GetServer()->GetIOContext());

    mState = EModuleState::INITIALIZED;
}

void UEventModule::Stop() {
    if (mState == EModuleState::STOPPED)
        return;
    mState = EModuleState::STOPPED;

    mServiceTimer->cancel();
    mPlayerTimer->cancel();
}

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

    RemoveExpiredServices();
    RemoveExpiredPlayers();
}

void UEventModule::ServiceListenEvent(const int64_t sid, const weak_ptr<IAgentBase> &weakPtr, const int event) {
    if (mState < EModuleState::INITIALIZED)
        return;

    if (sid < 0 || event < 0)
        return;

    {
        std::unique_lock lock(mServiceListenerMutex);
        mServiceListenerMap[event].insert_or_assign(sid, weakPtr);
    }

    RemoveExpiredServices();
}

void UEventModule::RemoveServiceListenerByEvent(const int64_t sid, const int event) {
    if (mState < EModuleState::INITIALIZED || GetServer()->GetState() == EServerState::TERMINATED)
        return;

    if (sid < 0 || event < 0)
        return;

    {
        std::unique_lock lock(mServiceListenerMutex);
        const auto iter = mServiceListenerMap.find(event);
        if (iter == mServiceListenerMap.end())
            return;

        iter->second.erase(sid);

        if (iter->second.empty())
            mServiceListenerMap.erase(iter);
    }

    RemoveExpiredServices();
}

void UEventModule::RemoveServiceListener(const int64_t sid) {
    if (mState < EModuleState::INITIALIZED || GetServer()->GetState() == EServerState::TERMINATED)
        return;

    if (sid < 0)
        return;

    {
        std::unique_lock lock(mServiceListenerMutex);
        for (auto &val: mServiceListenerMap | std::views::values) {
            val.erase(sid);
        }
    }

    RemoveExpiredServices();
}

void UEventModule::PlayerListenEvent(const int64_t pid, const weak_ptr<IAgentBase> &weakPtr, const int event) {
    if (mState < EModuleState::INITIALIZED)
        return;

    if (pid <= 0 || event < 0)
        return;

    {
        std::unique_lock lock(mPlayerListenerMutex);
        mPlayerListenerMap[event].insert_or_assign(pid, weakPtr);
    }

    RemoveExpiredPlayers();
}

void UEventModule::RemovePlayerListenerByEvent(const int64_t pid, const int event) {
    if (mState < EModuleState::INITIALIZED || GetServer()->GetState() == EServerState::TERMINATED)
        return;

    if (pid <= 0 || event < 0)
        return;

    {
        std::unique_lock lock(mPlayerListenerMutex);
        const auto iter = mPlayerListenerMap.find(event);
        if (iter == mPlayerListenerMap.end())
            return;

        iter->second.erase(pid);

        if (iter->second.empty())
            mPlayerListenerMap.erase(iter);
    }

    RemoveExpiredPlayers();
}

void UEventModule::RemovePlayerListener(const int64_t pid) {
    if (mState < EModuleState::INITIALIZED || GetServer()->GetState() == EServerState::TERMINATED)
        return;

    if (pid <= 0)
        return;

    {
        std::unique_lock lock(mPlayerListenerMutex);
        for (auto &val: mPlayerListenerMap | std::views::values) {
            val.erase(pid);
        }
    }

    RemoveExpiredPlayers();
}

void UEventModule::RemoveExpiredServices() {
    if (bServiceWaiting)
        return;

    bServiceWaiting = true;
    co_spawn(GetServer()->GetIOContext(), [this]() mutable -> awaitable<void> {
        try {
            mServiceTimer->expires_after(std::chrono::seconds(1));
            auto [ec] = co_await mServiceTimer->async_wait();
            if (ec)
                co_return;

            std::unique_lock lock(mServiceListenerMutex);
            for (auto &type : mServiceListenerMap | std::views::values) {
                for (auto iter = type.begin(); iter != type.end();) {
                    if (iter->second.expired()) {
                        type.erase(iter++);
                        continue;
                    }
                    ++iter;
                }
            }

            bServiceWaiting = false;
        } catch (const std::exception &e) {
            SPDLOG_ERROR("{} - {}", __FUNCTION__, e.what());
        }
    }, detached);
}

void UEventModule::RemoveExpiredPlayers() {
    if (bPlayerWaiting)
        return;

    bPlayerWaiting = true;
    co_spawn(GetServer()->GetIOContext(), [this]() mutable -> awaitable<void> {
        try {
            mPlayerTimer->expires_after(std::chrono::seconds(1));
            auto [ec] = co_await mPlayerTimer->async_wait();
            if (ec)
                co_return;

            std::unique_lock lock(mPlayerListenerMutex);
            for (auto &type : mPlayerListenerMap | std::views::values) {
                for (auto iter = type.begin(); iter != type.end();) {
                    if (iter->second.expired()) {
                        type.erase(iter++);
                        continue;
                    }
                    ++iter;
                }
            }

            bPlayerWaiting = false;
        } catch (const std::exception &e) {
            SPDLOG_ERROR("{} - {}", __FUNCTION__, e.what());
        }
    }, detached);
}
