#pragma once

#include "Module.h"
#include "base/Types.h"
#include "base/ContextHandle.h"

#include <absl/container/flat_hash_set.h>
#include <shared_mutex>


class BASE_API UTimerModule final : public IModuleBase {

    DECLARE_MODULE(UTimerModule)

    using ATickerSet = absl::flat_hash_set<FContextHandle, FContextHandle::FHash, FContextHandle::FEqual>;

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

private:
#pragma region Update
    std::unique_ptr<ASteadyTimer> mTickTimer;
    ATickerSet mTickers;
    mutable std::shared_mutex mTickMutex;
#pragma endregion
};

