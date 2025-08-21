#include "TickerModule.h"
#include "Server.h"
#include "Context.h"

#include <spdlog/spdlog.h>


UTickerModule::UTickerModule() {
}

void UTickerModule::Initial() {
    if (mState != EModuleState::CREATED)
        return;

    mTickTimer = std::make_unique<ASteadyTimer>(GetServer()->GetWorkerContext());

    mState = EModuleState::INITIALIZED;
}

void UTickerModule::Start() {
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

UTickerModule::~UTickerModule() {
    Stop();
}

void UTickerModule::AddTicker(const FContextHandle &handle) {
    if (mState != EModuleState::RUNNING)
        return;

    if (!handle.IsValid())
        return;

    std::unique_lock lock(mTickMutex);
    mTickers.insert(handle);
}

void UTickerModule::RemoveTicker(const FContextHandle &handle) {
    if (mState != EModuleState::RUNNING)
        return;

    if (handle < 0)
        return;

    std::unique_lock lock(mTickMutex);
    mTickers.erase(handle);
}

void UTickerModule::Stop() {
    if (mState == EModuleState::STOPPED)
        return;

    mState = EModuleState::STOPPED;
    mTickTimer->cancel();
}
