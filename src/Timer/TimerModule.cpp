#include "TimerModule.h"
#include "Server.h"
#include "Gateway/Gateway.h"
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

            std::unique_lock lock(mTickMutex);
            for (auto iter = mTickers.begin(); iter != mTickers.end();) {
                if (iter->expired()) {
                    iter = mTickers.erase(iter);
                } else {
                    iter->lock()->PushTicker(tickPoint, delta);
                    ++iter;
                }
            }
        }
    }, detached);

    mState = EModuleState::RUNNING;
}

UTimerModule::~UTimerModule() {
    Stop();
}

void UTimerModule::AddTicker(const std::weak_ptr<IContextBase> &ticker) {
    if (mState != EModuleState::RUNNING)
        return;

    if (ticker.expired())
        return;

    std::unique_lock lock(mTickMutex);
    mTickers.insert(ticker);
}

int64_t UTimerModule::CreateTimer(const std::weak_ptr<IContextBase> &wPtr, const ATimerTask &task, int delay, int rate) {
    if (mState != EModuleState::INITIALIZED || mState != EModuleState::RUNNING)
        return -1;

    if (wPtr.expired() || task == nullptr)
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
        mTimerMap[tid] = { wPtr, timer };
    }

    co_spawn(GetServer()->GetIOContext(), [this, tid, timer, wPtr, task, delay, rate]() -> awaitable<void> {
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

                if (const auto ctx = wPtr.lock()) {
                    ctx->PushTask(task);
                    continue;
                }
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

    std::unique_lock lock(mTimerMutex);
    const auto iter = mTimerMap.find(tid);
    if (iter == mTimerMap.end())
        return;

    iter->second.timer->cancel();
    mTimerMap.erase(iter);
}

void UTimerModule::CancelTimer(const std::weak_ptr<IContextBase> &wPtr) {
    if (mState != EModuleState::RUNNING)
        return;

    if (wPtr.expired())
        return;

    std::unique_lock lock(mTimerMutex);
    for (auto iter = mTimerMap.begin(); iter != mTimerMap.end();) {
        if (iter->second.wPointer.expired()) {
            iter->second.timer->cancel();
            iter = mTimerMap.erase(iter);
            mAllocator.RecycleTS(iter->first);
            continue;
        }

        if (iter->second.wPointer.lock().get() == wPtr.lock().get()) {
            iter->second.timer->cancel();
            iter = mTimerMap.erase(iter);
            mAllocator.RecycleTS(iter->first);
            continue;
        }

        ++iter;
    }
}

void UTimerModule::RemoveTimer(const int64_t tid) {
    std::unique_lock lock(mTimerMutex);
    for (auto iter = mTimerMap.begin(); iter != mTimerMap.end();) {
        if (iter->second.wPointer.expired()) {
            iter->second.timer->cancel();
            iter = mTimerMap.erase(iter);
            mAllocator.RecycleTS(iter->first);
            continue;
        }
        ++iter;
    }
    mTimerMap.erase(tid);
    mAllocator.RecycleTS(tid);
}

void UTimerModule::Stop() {
    if (mState == EModuleState::STOPPED)
        return;

    mState = EModuleState::STOPPED;
    mTickTimer->cancel();

    for (const auto &[wPointer, timer]: mTimerMap | std::views::values) {
        timer->cancel();
    }
}
