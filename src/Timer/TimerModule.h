#pragma once

#include "Module.h"
#include "Utils.h"
#include "Base/Types.h"
#include "Base/IdentAllocator.h"

#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <shared_mutex>

class IServiceBase;
class UContextBase;

class BASE_API UTimerModule final : public IModuleBase {

    DECLARE_MODULE(UTimerModule)

    using ATickerSet = std::unordered_set<std::weak_ptr<UContextBase>, FWeakPointerHash<UContextBase>, FWeakPointerEqual<UContextBase>>;
    using ATimerTask = std::function<void(IServiceBase *)>;

    struct FTimerNode {
        std::weak_ptr<UContextBase> wPointer;
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

    void AddTicker(const std::weak_ptr<UContextBase> &ticker);
    void RemoveTicker(const std::weak_ptr<UContextBase> &ticker);

    int64_t CreateTimer(const std::weak_ptr<UContextBase> &wPtr, const ATimerTask &task, int delay, int rate = -1);

    void CancelTimer(int64_t tid);
    void CancelTimer(const std::weak_ptr<UContextBase> &wPtr);

private:
    void RemoveTimer(int64_t tid);

private:
    TIdentAllocator<int64_t, true> mAllocator;
    ATimerMap mTimerMap;
    mutable std::shared_mutex mTimerMutex;

#pragma region Update
    std::unique_ptr<ASteadyTimer> mTickTimer;
    ATickerSet mTickers;
    mutable std::shared_mutex mTickMutex;
#pragma endregion
};

