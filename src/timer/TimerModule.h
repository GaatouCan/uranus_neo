#pragma once

#include "Module.h"
#include "base/Types.h"
#include "base/ContextHandle.h"
#include "base/IdentAllocator.h"

#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <shared_mutex>


class IServiceBase;
class UContext;


class BASE_API UTimerModule final : public IModuleBase {

    DECLARE_MODULE(UTimerModule)

    using ATickerSet = std::unordered_set<FContextHandle, FContextHandle::FHash, FContextHandle::FEqual>;
    using ATimerTask = std::function<void(IServiceBase *)>;

    struct FTimerNode {
        FContextHandle handle;
        std::shared_ptr<ASteadyTimer> timer;
    };

    using ATimerMap = std::unordered_map<int64_t, FTimerNode>;

protected:
    UTimerModule();

    void Initial() override;
    void Start() override;
    void Stop() override;

public:
    ~UTimerModule() override;

    constexpr const char *GetModuleName() const override {
        return "Timer Module";
    }

    void AddTicker(const FContextHandle &handle);
    void RemoveTicker(const FContextHandle &handle);

    int64_t CreateTimer(const FContextHandle &handle, const ATimerTask &task, int delay, int rate = -1);

    void CancelTimer(int64_t tid);
    void CancelTimer(const FContextHandle &handle);

private:
    void RemoveTimer(int64_t tid);

private:
#pragma region Timer
    TIdentAllocator<int64_t, true> mAllocator;
    ATimerMap mTimerMap;
    mutable std::shared_mutex mTimerMutex;
#pragma endregion

#pragma region Update
    std::unique_ptr<ASteadyTimer> mTickTimer;
    ATickerSet mTickers;
    mutable std::shared_mutex mTickMutex;
#pragma endregion
};

