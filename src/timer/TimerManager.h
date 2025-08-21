#pragma once

#include "TimerHandle.h"
#include "base/Types.h"
#include "base/IdentAllocator.h"

#include <shared_mutex>
#include <absl/container/flat_hash_map.h>


using ATimerTask = std::function<void(ASteadyTimePoint, ASteadyDuration)>;

class BASE_API UTimerManager final {

    friend class UTimer;

public:
    UTimerManager() = delete;

    explicit UTimerManager(asio::io_context& ctx);
    ~UTimerManager();

    DISABLE_COPY_MOVE(UTimerManager)

    [[nodiscard]] asio::io_context& GetIOContext() const;

    [[nodiscard]] FTimerHandle CreateTimer();
    [[nodiscard]] FTimerHandle CreateTimer(const ATimerTask &task, int delay, int rate = -1);

    template<class Target, class Functor, class ... Args>
    FTimerHandle CreateTimerT(int delay, int rate, Functor && func, Target *obj, Args &&... args);

    [[nodiscard]] FTimerHandle FindTimer(int64_t tid) const;

    void CancelTimer(int64_t tid) const;
    void CancelAll();

private:
    void RemoveTimer(int64_t tid);

private:
    asio::io_context& mContext;
    TIdentAllocator<int64_t, true> mAllocator;

    absl::flat_hash_map<FTimerHandle, shared_ptr<UTimer>, FTimerHandle::FHash, FTimerHandle::FEqual> mTimerMap;
    mutable std::shared_timed_mutex mMutex;
};

template<class Target, class Functor, class ... Args>
inline FTimerHandle UTimerManager::CreateTimerT(int delay, int rate, Functor &&func, Target *obj, Args &&...args) {
    if (obj == nullptr)
        return {};

    auto task = [func = std::forward<Functor>(func), obj, ...args = std::forward<Args>(args)](ASteadyTimePoint point, ASteadyDuration delta) {
        std::invoke(func, obj, point, delta, std::forward<Args>(args)...);
    };

    return this->CreateTimer(task, delay, rate);
}
