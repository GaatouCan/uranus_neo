#include "TimerModule.h"
#include "Server.h"
#include "ContextBase.h"

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

            std::unique_lock lock(mTickMutex);
            for (auto iter = mTickers.begin(); iter != mTickers.end();) {
                if (const auto ctx = iter->Get()) {
                    ctx->PushTicker(tickPoint, delta);
                    ++iter;
                } else {
                    iter = mTickers.erase(iter);
                }
            }
        }
    }, detached);

    mState = EModuleState::RUNNING;
}

UTimerModule::~UTimerModule() {
    Stop();
}

void UTimerModule::AddTicker(const FContextHandle &handle) {
    if (mState != EModuleState::RUNNING)
        return;

    if (!handle.IsValid())
        return;

    std::unique_lock lock(mTickMutex);
    mTickers.insert(handle);
}

void UTimerModule::RemoveTicker(const FContextHandle &handle) {
    if (mState != EModuleState::RUNNING)
        return;

    if (handle < 0)
        return;

    std::unique_lock lock(mTickMutex);
    mTickers.erase(handle);
}

int64_t UTimerModule::CreateTimer(const FContextHandle &handle, const ATimerTask &task, int delay, int rate) {
    if (mState != EModuleState::INITIALIZED || mState != EModuleState::RUNNING)
        return -1;

    if (!handle.IsValid() || task == nullptr)
        return -2;

    const auto tid = mAllocator.AllocateTS();
    if (tid < 0)
        return -3;

    std::shared_ptr<ASteadyTimer> timer;

    {
        std::unique_lock lock(mTimerMutex);

        if (mTimerMap.contains(tid))
            return {};

        timer = std::make_shared<ASteadyTimer>(GetServer()->GetIOContext());
        mTimerMap[tid] = { handle, timer };
    }

    co_spawn(GetServer()->GetIOContext(), [this, tid, timer, handle, task, delay, rate]() -> awaitable<void> {
        try {
            auto point = std::chrono::steady_clock::now() + std::chrono::milliseconds(delay * 100);

            do {
                timer->expires_at(point);
                if (rate > 0) {
                    point += std::chrono::milliseconds(rate * 100);
                }

                if (auto [ec] = co_await timer->async_wait(); ec) {
                    SPDLOG_DEBUG("{:<20} - Timer[{}] Canceled", "UTimerModule::SetTimer()", tid);
                    break;
                }

                if (const auto ctx = handle.Get()) {
                    ctx->PushTask(task);
                    continue;
                }

                // If The Inner Pointer Of Handle Expired, Break The Loop And End The Timer
                break;
            } while (rate > 0 && mState == EModuleState::RUNNING);
        } catch (const std::exception &e) {
            SPDLOG_ERROR("{:<20} - Exception: {}", "UTimerModule::CreateTimer", e.what());
        }

        RemoveTimer(tid);
    }, detached);

    return tid;
}

void UTimerModule::CancelTimer(const int64_t tid) {
    if (mState != EModuleState::RUNNING)
        return;

    std::shared_ptr<ASteadyTimer> timer;

    {
        std::unique_lock lock(mTimerMutex);
        const auto iter = mTimerMap.find(tid);
        if (iter == mTimerMap.end())
            return;

        timer = iter->second.timer;
        mTimerMap.erase(iter);
    }

    timer->cancel();
}

void UTimerModule::CancelTimer(const FContextHandle &handle) {
    if (mState != EModuleState::RUNNING)
        return;

    if (handle < 0)
        return;

    std::vector<std::shared_ptr<ASteadyTimer>> del;

    {
        std::unique_lock lock(mTimerMutex);
        for (auto iter = mTimerMap.begin(); iter != mTimerMap.end();) {
            if (iter->second.handle == handle) {
                del.emplace_back(iter->second.timer);
                iter = mTimerMap.erase(iter);
            } else {
                ++iter;
            }
        }
    }

    for (const auto &timer : del) {
        timer->cancel();
    }
}

void UTimerModule::RemoveTimer(const int64_t tid) {
    std::unique_lock lock(mTimerMutex);
    mTimerMap.erase(tid);
    mAllocator.RecycleTS(tid);
}

void UTimerModule::Stop() {
    if (mState == EModuleState::STOPPED)
        return;

    mState = EModuleState::STOPPED;
    mTickTimer->cancel();

    for (const auto &[handle, timer]: mTimerMap | std::views::values) {
        timer->cancel();
    }
}
