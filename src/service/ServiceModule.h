#pragma once

#include "Module.h"
#include "base/Types.h"
#include "base/SingleIOContextPool.h"
#include "base/IdentAllocator.h"
#include "factory/ServiceFactory.h"

#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>
#include <shared_mutex>


using std::unique_ptr;
using std::shared_ptr;
using std::make_unique;
using std::make_shared;

class UServiceAgent;
class IDataAsset_Interface;

class BASE_API UServiceModule final : public IModuleBase {

    DECLARE_MODULE(UServiceModule)

public:
    UServiceModule();
    ~UServiceModule() override;

    [[nodiscard]] constexpr const char *GetModuleName() const override {
        return "Service Module";
    }

    template<class T, class... Args>
    requires std::derived_from<T, IServiceFactory_Interface>
    void SetServiceFactory(Args && ... args) {
        if (mState != EModuleState::CREATED)
            throw std::logic_error("Only can set ServiceFactory in CREATED state");

        mServiceFactory = make_unique<T>(std::forward<Args>(args)...);
    }

    [[nodiscard]] asio::io_context &GetWorkerIOContext();

    [[nodiscard]] shared_ptr<UServiceAgent> FindService(int64_t sid) const;
    [[nodiscard]] shared_ptr<UServiceAgent> FindService(const std::string &name) const;

    void BootService(const std::string &path, IDataAsset_Interface *pData);

    void ShutdownService(int64_t sid);
    void ShutdownService(const std::string &name);

    void AddTicker(int64_t sid);
    void RemoveTicker(int64_t sid);

protected:
    void Initial() override;
    void Start() override;
    void Stop() override;

private:
    awaitable<void> UpdateLoop(int ms);

private:
    USingleIOContextPool mWorkerPool;

    absl::flat_hash_map<int64_t, shared_ptr<UServiceAgent>> mServiceMap;
    mutable std::shared_mutex mServiceMutex;

    absl::flat_hash_map<std::string, int64_t> mServiceNameMap;
    mutable std::shared_mutex mServiceNameMutex;

    unique_ptr<IServiceFactory_Interface> mServiceFactory;
    TIdentAllocator<int64_t, true> mAllocator;

    std::unique_ptr<ASteadyTimer> mTickTimer;

    absl::flat_hash_set<int64_t> mTickerSet;
    mutable std::shared_mutex mTickMutex;
};
