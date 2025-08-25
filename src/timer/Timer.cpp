#include "Timer.h"
#include "TimerManager.h"

#include <spdlog/spdlog.h>


UTimer::UTimer(UTimerManager *manager)
    : mManager(manager),
      mCtx(manager->GetIOContext()),
      mID(-1),
      mInnerTimer(mCtx),
      mDelay(-1),
      mRate(-1) {
}

UTimer::~UTimer() {
    Cancel();
}

asio::io_context &UTimer::GetIOContext() const {
    return mManager->GetIOContext();
}

FTimerHandle UTimer::GetTimerHandle() {
    if (mID < 0)
        return {};

    return FTimerHandle{ mID, weak_from_this() };
}

void UTimer::Start() {
    co_spawn(mCtx, [self = shared_from_this()]() mutable -> awaitable<void> {
        try {
            auto point = std::chrono::steady_clock::now();
            ASteadyDuration delta;

            if (self->mDelay > 0) {
                delta = std::chrono::milliseconds(self->mDelay * 100);
                point += delta;

                self->mInnerTimer.expires_at(point);
            } else {
                std::invoke(self->mTask, point, delta);

                delta = std::chrono::milliseconds(self->mRate * 100);
                point += delta;

                self->mInnerTimer.expires_at(point);
            }

            while (self->mRate > 0) {
                if (auto [ec] = co_await self->mInnerTimer.async_wait(); ec) {
                    SPDLOG_DEBUG("{:<20} - Timer[{}] Canceled", "UTimer::Start()", self->mID);
                    break;
                }

                std::invoke(self->mTask, point, delta);

                delta = std::chrono::milliseconds(self->mRate * 100);
                point += delta;
                self->mInnerTimer.expires_at(point);
            }
        } catch (const std::exception &e) {
            SPDLOG_ERROR("{:<20} - Exception: {}", "UTimerModule::CreateTimer", e.what());
        }

        if (self->mManager) {
            self->mManager->RemoveTimer(self->mID);
        }
    }, detached);
}

void UTimer::Cancel() {
    mInnerTimer.cancel();
}

void UTimer::SetUpID(const int64_t id) {
    mID = id;
}

void UTimer::CleanUpManager() {
    mManager = nullptr;
}

void UTimer::SetDelay(const int delay) {
    mDelay = delay;
}

void UTimer::SetRate(const int rate) {
    mRate = rate;
}

void UTimer::SetTask(const ATimerTask &task) {
    mTask = task;
}
