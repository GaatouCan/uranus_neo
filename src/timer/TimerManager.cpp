#include "TimerManager.h"
#include "Timer.h"

#include <ranges>

UTimerManager::UTimerManager(asio::io_context &ctx)
    : mContext(ctx) {
}

UTimerManager::~UTimerManager() {
    for (const auto &timer : mTimerMap | std::views::values) {
        timer->CleanUpManager();
        timer->Cancel();
    }
}

asio::io_context &UTimerManager::GetIOContext() const {
    return mContext;
}

FTimerHandle UTimerManager::CreateTimer() {
    const auto tid = mAllocator.AllocateTS();
    const auto timer = std::make_shared<UTimer>(this);
    timer->SetUpID(tid);

    std::unique_lock lock(mMutex);
    if (!mTimerMap.contains(tid)) {
        const auto handle = timer->GetTimerHandle();
        mTimerMap.insert_or_assign(handle, timer);
        return handle;
    }

    return {};
}

FTimerHandle UTimerManager::CreateTimer(const ATimerTask &task, const int delay, const int rate) {
    const auto handle = CreateTimer();
    if (!handle.IsValid())
        return {};

    const auto timer = handle.timer.lock();
    if (timer == nullptr) [[unlikely]]
        return {};

    timer->SetDelay(delay);
    timer->SetRate(rate);
    timer->SetTask(task);

    return handle;
}

FTimerHandle UTimerManager::FindTimer(const int64_t tid) const {
    std::shared_lock lock(mMutex);
    const auto iter = mTimerMap.find(tid);
    return iter == mTimerMap.end() ? FTimerHandle() : iter->first;
}

void UTimerManager::CancelTimer(const int64_t tid) const {
    if (const auto handle = FindTimer(tid); handle.IsValid()) {
        if (const auto timer = handle.timer.lock()) {
            timer->Cancel();
        }
    }
}

void UTimerManager::CancelAll() {
    std::unique_lock lock(mMutex);
    for (const auto &timer : mTimerMap | std::views::values) {
        timer->CleanUpManager();
        timer->Cancel();
    }
}

void UTimerManager::RemoveTimer(const int64_t tid) {
    std::unique_lock lock(mMutex);
    if (const auto it = mTimerMap.find(tid); it != mTimerMap.end()) {
        mTimerMap.erase(it);
        mAllocator.RecycleTS(tid);
    }
}
