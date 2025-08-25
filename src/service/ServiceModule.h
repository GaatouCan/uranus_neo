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

/**
 * Manager All The Services
 */
class BASE_API UServiceModule final : public IModuleBase {

    DECLARE_MODULE(UServiceModule)

    /** The Worker Pool For All The Service **/
    USingleIOContextPool mWorkerPool;

    /** All The Service Map **/
    absl::flat_hash_map<int64_t, shared_ptr<UServiceAgent>> mServiceMap;
    mutable std::shared_mutex mServiceMutex;

    /** Service Name To Service ID Mapping **/
    absl::flat_hash_map<std::string, int64_t> mServiceNameMap;
    mutable std::shared_mutex mServiceNameMutex;

    /** The Service Factory Use To Load Service Shared Libraries **/
    unique_ptr<IServiceFactory_Interface> mServiceFactory;

    /** Allocate Service ID **/
    TIdentAllocator<int64_t, true> mAllocator;

    /** For Update Per Tick **/
    std::unique_ptr<ASteadyTimer> mTickTimer;

    /** All The Service ID Which Need To Update **/
    absl::flat_hash_set<int64_t> mTickerSet;
    mutable std::shared_mutex mTickMutex;

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

    /// Get Reference Of IOContext In Worker Pool
    [[nodiscard]] asio::io_context &GetWorkerIOContext();

    /// Find Service By Service ID
    [[nodiscard]] shared_ptr<UServiceAgent> FindService(int64_t sid) const;

    /// Find Service By Service Name
    [[nodiscard]] shared_ptr<UServiceAgent> FindService(const std::string &name) const;

    /// Boot Extend Service
    void BootService(const std::string &name, IDataAsset_Interface *pData);

    /// Shutdown Extend Service By Service ID
    void ShutdownService(int64_t sid);

    /// Shutdown Extend Service By Service Name
    void ShutdownService(const std::string &name);

    /// Register Service To Update
    void InsertTicker(int64_t sid);

    /// Unregister Service To Update
    void RemoveTicker(int64_t sid);

    [[nodiscard]] std::map<int64_t, std::string> GetAllServiceMap() const;

    void ForeachService(const std::function<bool(const shared_ptr<UServiceAgent> &)> &func) const;

protected:
    void Initial() override;
    void Start() override;
    void Stop() override;

private:
    awaitable<void> UpdateLoop(int ms);
};
