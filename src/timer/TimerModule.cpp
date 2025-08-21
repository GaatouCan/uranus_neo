#include "TimerModule.h"
#include "Server.h"
#include "Context.h"

#include <spdlog/spdlog.h>


UTimerModule::UTimerModule() {
}

void UTimerModule::Initial() {
    if (mState != EModuleState::CREATED)
        return;

    mTickTimer = std::make_unique<ASteadyTimer>(GetServer()->GetWorkerContext());

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
                    mTickers.erase(iter++);
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

void UTimerModule::Stop() {
    if (mState == EModuleState::STOPPED)
        return;

    mState = EModuleState::STOPPED;
    mTickTimer->cancel();
}
