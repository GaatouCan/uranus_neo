#include "TimerModule.h"
#include "Server.h"
#include "Gateway/Gateway.h"
#include "Gateway/AgentContext.h"
#include "Service/ServiceModule.h"
#include "Service/ServiceContext.h"

#include <spdlog/spdlog.h>
#include <ranges>


UTimerModule::UTimerModule() {
}

void UTimerModule::Initial() {
    if (mState != EModuleState::CREATED)
        return;

    mTickTimer = std::make_unique<ASteadyTimer>(GetServer()->GetIOContext());

    mState = EModuleState::INITIALIZED;
}

void UTimerModule::Start() {
    if (mState != EModuleState::INITIALIZED)
        return;

    co_spawn(GetServer()->GetIOContext(), [this]() -> awaitable<void> {
        ASteadyTimePoint tickPoint = std::chrono::steady_clock::now();
        constexpr ASteadyDuration delta = std::chrono::milliseconds(100);

        while (mState == EModuleState::RUNNING) {
            tickPoint += delta;
            mTickTimer->expires_at(tickPoint);

            if (const auto [ec] = co_await mTickTimer->async_wait(); ec) {
                break;
            }

            if (const auto *module = GetServer()->GetModule<UGateway>()) {
                for (const auto &pid : mPlayerTickSet) {
                    if (const auto agent = module->FindPlayerAgent(pid)) {
                        agent->PushTicker(tickPoint, delta);
                    }
                }
            }

            if (const auto *module = GetServer()->GetModule<UServiceModule>()) {
                for (const auto &sid : mServiceTickSet) {
                    if (const auto service = module->FindService(sid)) {
                        service->PushTicker(tickPoint, delta);
                    }
                }
            }
        }
    }, detached);

    mState = EModuleState::RUNNING;
}

UTimerModule::~UTimerModule() {
    Stop();
}

FTimerHandle UTimerModule::SetSteadyTimer(const int32_t sid, const int64_t pid, const ATimerTask &task, int delay, int rate) {
    const auto id = mAllocator.AllocateTS();
    if (id < 0)
        return { -1, true };

    {
        std::unique_lock lock(mTimerMutex);

        if (mSteadyTimerMap.contains(id))
            return { -2, true };

        FSteadyTimerNode node { sid, pid, make_shared<ASteadyTimer>(GetServer()->GetIOContext()) };
        mSteadyTimerMap.emplace(id, std::move(node));

        if (sid > 0)
            mServiceToSteadyTimer[sid].emplace(id);
        else
            mPlayerToSteadyTimer[pid].emplace(id);
    }

    co_spawn(GetServer()->GetIOContext(), [this, timer = mSteadyTimerMap[id].timer, id, sid, pid, delay, rate, task]() -> awaitable<void> {
        try {
            auto point = std::chrono::steady_clock::now() + std::chrono::milliseconds(delay * 100);

            do {
                timer->expires_at(point);
                if (rate > 0) {
                    point += std::chrono::milliseconds(rate * 100);
                }

                if (auto [ec] = co_await timer->async_wait(); ec) {
                    SPDLOG_DEBUG("{:<20} - Timer[{}] Canceled", "UTimerModule::SetTimer()", id);
                    break;
                }

                if (sid > 0) {
                    if (const auto *service = GetServer()->GetModule<UServiceModule>()) {
                        if (const auto context = service->FindService(sid)) {
                            context->PushTask(task);
                            continue;
                        }
                    }
                } else {
                    if (const auto *gateway = GetServer()->GetModule<UGateway>()) {
                        if (const auto player = gateway->FindPlayerAgent(pid)) {
                            player->PushTask(task);
                            continue;
                        }
                    }
                }

                break;
            } while (rate > 0 && mState == EModuleState::RUNNING);
        } catch (const std::exception &e) {
            SPDLOG_ERROR("{:<20} - Exception: {}", "UTimerModule::SetTimer", e.what());
        }

        RemoveSteadyTimer(id);
        mAllocator.RecycleTS(id);
    }, detached);

    return { id, true };
}

FTimerHandle UTimerModule::SetSystemTimer(int32_t sid, int64_t pid, const ATimerTask &task, int delay, int rate) {
    const auto id = mAllocator.AllocateTS();
    if (id < 0)
        return { -1, false };

    {
        std::unique_lock lock(mTimerMutex);

        if (mSteadyTimerMap.contains(id))
            return { -2,  true };

        FSystemTimerNode node { sid, pid, make_shared<ASystemTimer>(GetServer()->GetIOContext()) };
        mSystemTimerMap.emplace(id, std::move(node));

        if (sid > 0)
            mServiceToSystemTimer[sid].emplace(id);
        else
            mPlayerToSystemTimer[pid].emplace(id);
    }

    co_spawn(GetServer()->GetIOContext(), [this, timer = mSystemTimerMap[id].timer, id, sid, pid, delay, rate, task]() -> awaitable<void> {
        try {
            auto point = std::chrono::system_clock::now() + std::chrono::milliseconds(delay * 100);

            do {
                timer->expires_at(point);
                if (rate > 0) {
                    point += std::chrono::milliseconds(rate * 100);
                }

                if (auto [ec] = co_await timer->async_wait(); ec) {
                    SPDLOG_DEBUG("{:<20} - Timer[{}] Canceled", "UTimerModule::SetTimer()", id);
                    break;
                }

                if (sid > 0) {
                    if (const auto *service = GetServer()->GetModule<UServiceModule>()) {
                        if (const auto context = service->FindService(sid)) {
                            context->PushTask(task);
                            continue;
                        }
                    }
                } else {
                    if (const auto *gateway = GetServer()->GetModule<UGateway>()) {
                        if (const auto player = gateway->FindPlayerAgent(pid)) {
                            player->PushTask(task);
                            continue;
                        }
                    }
                }

                // If Target Not Exist, Stop The Timer
                break;
            } while (rate > 0 && mState == EModuleState::RUNNING);
        } catch (const std::exception &e) {
            SPDLOG_ERROR("{:<20} - Exception: {}", "UTimerModule::SetTimer", e.what());
        }

        RemoveSystemTimer(id);
        mAllocator.RecycleTS(id);
    }, detached);

    return { id, false };
}

void UTimerModule::CancelTimer(const FTimerHandle &handle) {
    if (mState != EModuleState::RUNNING)
        return;

    std::unique_lock lock(mTimerMutex);
    if (handle.bSteady) {
        const auto iter = mSteadyTimerMap.find(handle.id);
        if (iter == mSteadyTimerMap.end())
            return;

        iter->second.timer->cancel();

        if (iter->second.sid > 0) {
            if (const auto it = mServiceToSteadyTimer.find(iter->second.sid); it != mServiceToSteadyTimer.end())
                it->second.erase(handle.id);
        } else {
            if (const auto it = mPlayerToSteadyTimer.find(iter->second.pid); it != mPlayerToSteadyTimer.end())
                it->second.erase(handle.id);
        }

        mSteadyTimerMap.erase(iter);
    } else {
        const auto iter = mSystemTimerMap.find(handle.id);
        if (iter == mSystemTimerMap.end())
            return;

        iter->second.timer->cancel();

        if (iter->second.sid > 0) {
            if (const auto it = mServiceToSystemTimer.find(iter->second.sid); it != mServiceToSystemTimer.end())
                it->second.erase(handle.id);
        } else {
            if (const auto it = mPlayerToSystemTimer.find(iter->second.pid); it != mPlayerToSystemTimer.end())
                it->second.erase(handle.id);
        }

        mSystemTimerMap.erase(iter);
    }
}

void UTimerModule::CancelServiceTimer(const int32_t sid) {
    if (mState != EModuleState::RUNNING)
        return;

    std::unique_lock lock(mTimerMutex);

    // Clean Steady Timers
    const auto steadyIter = mServiceToSteadyTimer.find(sid);
    if (steadyIter == mServiceToSteadyTimer.end())
        return;

    for (const auto tid : steadyIter->second) {
        const auto timerIter = mSteadyTimerMap.find(tid);
        if (timerIter == mSteadyTimerMap.end())
            continue;

        timerIter->second.timer->cancel();
        mSteadyTimerMap.erase(tid);
    }

    // Clean System Timers
    const auto systemIter = mServiceToSystemTimer.find(sid);
    if (systemIter == mServiceToSystemTimer.end())
        return;

    for (const auto tid : systemIter->second) {
        const auto timerIter = mSystemTimerMap.find(tid);
        if (timerIter == mSystemTimerMap.end())
            continue;

        timerIter->second.timer->cancel();
        mSystemTimerMap.erase(tid);
    }
}

void UTimerModule::CancelPlayerTimer(const int64_t pid) {
    if (mState != EModuleState::RUNNING)
        return;

    std::unique_lock lock(mTimerMutex);

    // Clean Steady Timers
    const auto steadyIter = mPlayerToSteadyTimer.find(pid);
    if (steadyIter == mPlayerToSteadyTimer.end())
        return;

    for (const auto tid : steadyIter->second) {
        const auto timerIter = mSteadyTimerMap.find(tid);
        if (timerIter == mSteadyTimerMap.end())
            continue;

        timerIter->second.timer->cancel();
        mSteadyTimerMap.erase(tid);
    }

    // Clean System Timers
    const auto systemIter = mPlayerToSystemTimer.find(pid);
    if (systemIter == mPlayerToSystemTimer.end())
        return;

    for (const auto tid : systemIter->second) {
        const auto timerIter = mSystemTimerMap.find(tid);
        if (timerIter == mSystemTimerMap.end())
            continue;

        timerIter->second.timer->cancel();
        mSystemTimerMap.erase(tid);
    }
}

void UTimerModule::AddUpdatePlayer(const int64_t pid) {
    if (mState != EModuleState::RUNNING)
        return;

    if (pid <= 0)
        return;

    std::unique_lock lock(mTickMutex);
    mPlayerTickSet.insert(pid);
}

void UTimerModule::AddUpdateService(const int32_t sid) {
    if (mState != EModuleState::RUNNING)
        return;

    if (sid <= 0)
        return;

    std::unique_lock lock(mTickMutex);
    mServiceTickSet.insert(sid);
}

void UTimerModule::RemoveUpdatePlayer(const int64_t pid) {
    if (mState != EModuleState::RUNNING)
        return;

    if (pid <= 0)
        return;

    std::unique_lock lock(mTickMutex);
    mPlayerTickSet.erase(pid);
}

void UTimerModule::RemoveUpdatePlayer(const std::set<int64_t> &set) {
    if (mState != EModuleState::RUNNING)
        return;

    std::unique_lock lock(mTickMutex);
    for (const auto &pid : set) {
        mPlayerTickSet.erase(pid);
    }
}

void UTimerModule::RemoveUpdateService(const int32_t sid) {
    if (mState != EModuleState::RUNNING)
        return;

    if (sid <= 0)
        return;

    std::unique_lock lock(mTickMutex);
    mServiceTickSet.erase(sid);
}

void UTimerModule::RemoveUpdateService(const std::set<int32_t> &set) {
    if (mState != EModuleState::RUNNING)
        return;

    std::unique_lock lock(mTickMutex);
    for (const auto &sid : set) {
        mServiceTickSet.erase(sid);
    }
}

void UTimerModule::Stop() {
    if (mState == EModuleState::STOPPED)
        return;

    mState = EModuleState::STOPPED;
    mTickTimer->cancel();

    for (const auto &val : mSteadyTimerMap | std::views::values) {
        val.timer->cancel();
    }

    for (const auto &val : mSystemTimerMap | std::views::values) {
        val.timer->cancel();
    }
}

void UTimerModule::RemoveSteadyTimer(const int64_t id) {
    std::unique_lock lock(mTimerMutex);
    const auto iter = mSteadyTimerMap.find(id);
    if (iter == mSteadyTimerMap.end())
        return;

    if (iter->second.sid > 0) {
        if (const auto it = mServiceToSteadyTimer.find(iter->second.sid); it != mServiceToSteadyTimer.end()) {
            it->second.erase(id);
        }
    } else {
        if (const auto it = mPlayerToSteadyTimer.find(iter->second.pid); it != mPlayerToSteadyTimer.end()) {
            it->second.erase(id);
        }
    }

    mSteadyTimerMap.erase(iter);
}

void UTimerModule::RemoveSystemTimer(const int64_t id) {
    std::unique_lock lock(mTimerMutex);
    const auto iter = mSystemTimerMap.find(id);
    if (iter == mSystemTimerMap.end())
        return;

    if (iter->second.sid > 0) {
        if (const auto it = mServiceToSystemTimer.find(iter->second.sid); it != mServiceToSystemTimer.end()) {
            it->second.erase(id);
        }
    } else {
        if (const auto it = mPlayerToSystemTimer.find(iter->second.pid); it != mPlayerToSystemTimer.end()) {
            it->second.erase(id);
        }
    }

    mSystemTimerMap.erase(iter);
}
