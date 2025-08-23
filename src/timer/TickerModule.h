#pragma once

#include "Module.h"
#include "base/Types.h"

#include <absl/container/flat_hash_map.h>
#include <memory>
#include <shared_mutex>


class UServiceAgent;
using std::weak_ptr;


class BASE_API UTickerModule final : public IModuleBase {

    DECLARE_MODULE(UTickerModule)

    using ATickerSet = absl::flat_hash_map<int64_t, weak_ptr<UServiceAgent>>;

protected:
    void Initial() override;
    void Start() override;
    void Stop() override;

public:
    UTickerModule();
    ~UTickerModule() override;

    constexpr const char *GetModuleName() const override {
        return "Timer Module";
    }

    void AddTicker(int64_t sid, const weak_ptr<UServiceAgent> &weakPtr);
    void RemoveTicker(int64_t sid);

private:
#pragma region Update
    std::unique_ptr<ASteadyTimer> mTickTimer;
    ATickerSet mTickers;
    mutable std::shared_mutex mTickMutex;
#pragma endregion
};

