#pragma once

#include "Module.h"
#include "Utils.h"
#include "IdentAllocator.h"

#include <functional>
#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>
#include <shared_mutex>


class IServiceBase;

struct BASE_API FTimerHandle {
    int64_t id;
    bool bSteady;
};

class BASE_API UTimerModule final : public IModuleBase {

    DECLARE_MODULE(UTimerModule)

    using ATimerTask = std::function<void(IServiceBase *)>;
    using AServiceToTimerMap = absl::flat_hash_map<int32_t, absl::flat_hash_set<int64_t>>;
    using APlayerToTimerMap = absl::flat_hash_map<int64_t, absl::flat_hash_set<int64_t>>;

    struct FSteadyTimerNode final {
        int32_t sid;
        int64_t pid;
        shared_ptr<ASteadyTimer> timer;
    };

    struct FSystemTimerNode final {
        int32_t sid;
        int64_t pid;
        shared_ptr<ASystemTimer> timer;
    };

protected:
    UTimerModule();

    void Stop() override;

public:
    ~UTimerModule() override;

    constexpr const char *GetModuleName() const override {
        return "Timer Module";
    }

    FTimerHandle SetSteadyTimer(int32_t sid, int64_t pid, const ATimerTask &task, int delay, int rate = -1);
    FTimerHandle SetSystemTimer(int32_t sid, int64_t pid, const ATimerTask &task, int delay, int rate = -1);

    void CancelTimer(const FTimerHandle &handle);

    void CancelServiceTimer(int32_t sid);
    void CancelPlayerTimer(int64_t pid);

private:
    void RemoveSteadyTimer(int64_t id);
    void RemoveSystemTimer(int64_t id);

private:
    TIdentAllocator<int64_t, true> mAllocator;

    absl::flat_hash_map<int64_t, FSteadyTimerNode> mSteadyTimerMap;
    absl::flat_hash_map<int64_t, FSystemTimerNode> mSystemTimerMap;

    AServiceToTimerMap mServiceToSteadyTimer;
    AServiceToTimerMap mServiceToSystemTimer;

    APlayerToTimerMap mPlayerToSteadyTimer;
    APlayerToTimerMap mPlayerToSystemTimer;

    mutable std::shared_mutex mTimerMutex;
};

