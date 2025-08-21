#include "TimerManager.h"
#include "Timer.h"


UTimerManager::UTimerManager(asio::io_context &ctx)
    : mContext(ctx) {
}

UTimerManager::~UTimerManager() {
}

asio::io_context &UTimerManager::GetIOContext() const {
    return mContext;
}

FTimerHandle UTimerManager::CreateTimer() {
    const auto tid = mAllocator.AllocateTS();
    const auto timer = std::make_shared<UTimer>(this);

    std::unique_lock lock(mMutex);
    if (!mTimerMap.contains(tid)) {
        const auto handle = timer->GetTimerHandle();
        mTimerMap.insert_or_assign(handle, timer);
        return handle;
    }

    return {};
}
