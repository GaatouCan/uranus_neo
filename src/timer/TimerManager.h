#pragma once

#include "TimerHandle.h"
#include "base/IdentAllocator.h"

#include <shared_mutex>
#include <absl/container/flat_hash_map.h>
#include <asio/io_context.hpp>

class BASE_API UTimerManager final {

public:
    UTimerManager() = delete;

    explicit UTimerManager(asio::io_context& ctx);
    ~UTimerManager();

    DISABLE_COPY_MOVE(UTimerManager)

    [[nodiscard]] asio::io_context& GetIOContext() const;

    [[nodiscard]] FTimerHandle CreateTimer();

private:
    asio::io_context& mContext;
    TIdentAllocator<int64_t, true> mAllocator;

    absl::flat_hash_map<FTimerHandle, shared_ptr<UTimer>, FTimerHandle::FHash, FTimerHandle::FEqual> mTimerMap;
    mutable std::shared_timed_mutex mMutex;
};
