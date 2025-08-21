#pragma once

#include "Common.h"
#include "base/Types.h"
#include "TimerHandle.h"

#include <functional>


class UTimerManager;
using ATimerTask = std::function<void(ASteadyTimePoint, ASteadyDuration)>;


class BASE_API UTimer final : public std::enable_shared_from_this<UTimer> {

    friend class UTimerManager;

public:
    UTimer() = delete;

    explicit UTimer(UTimerManager *manager);
    ~UTimer();

    DISABLE_COPY_MOVE(UTimer);

    [[nodiscard]] asio::io_context &GetIOContext() const;
    [[nodiscard]] FTimerHandle GetTimerHandle();

    void SetDelay(int delay);
    void SetRate(int rate);
    void SetTask(const ATimerTask &task);

    void Start();
    void Cancel();

    template<class Type, class Functor, class ... Args>
    void SetMethod(Functor && func, Type *obj, Args &&... args) {
        if (obj == nullptr)
            return;

        auto task = [func = std::forward<Functor>(func), obj, ...args = std::forward<Args>(args)](ASteadyTimePoint point, ASteadyDuration delta) {
            std::invoke(func, obj, point, delta, std::forward<Args>(args)...);
        };

        this->SetTask(task);
    };

private:
    void SetUpID(int64_t id);

private:
    UTimerManager *const mManager;

    int64_t mID;
    ASteadyTimer mInnerTimer;

    int mDelay;
    int mRate;

    ATimerTask mTask;
};
