#pragma once

#include "Module.h"
#include "ServiceType.h"
#include "Base/SharedLibrary.h"
#include "Base/IdentAllocator.h"
#include "Base/ContextHandle.h"

#include <unordered_map>
#include <map>
#include <unordered_set>
#include <shared_mutex>


using std::unordered_map;
using std::unordered_set;

class IDataAsset_Interface;
class UServiceContext;


class BASE_API UServiceModule final : public IModuleBase {

    DECLARE_MODULE(UServiceModule)

protected:
    UServiceModule();

    void Initial() override;
    void Start() override;
    void Stop() override;

public:
    ~UServiceModule() override;

    [[nodiscard]] constexpr const char *GetModuleName() const override {
        return "Service Module";
    }

    [[nodiscard]] std::shared_ptr<UServiceContext> FindService(FServiceHandle sid) const;
    [[nodiscard]] std::shared_ptr<UServiceContext> FindService(const std::string &name) const;

    [[nodiscard]] std::map<std::string, FServiceHandle> GetServiceList() const;
    [[nodiscard]] FServiceHandle GetServiceID(const std::string &name) const;

    [[nodiscard]] FSharedLibrary FindServiceLibrary(const std::string &filename, EServiceType type = EServiceType::EXTEND) const;

    void BootExtendService(const std::string &filename, const IDataAsset_Interface *data = nullptr);
    void ShutdownService(FServiceHandle sid);

    [[nodiscard]] FServiceHandle AcquireServiceID();
    void RecycleServiceID(FServiceHandle id);

private:
    void OnServiceShutdown(const std::string &filename, FServiceHandle sid, EServiceType type);

private:
    /** Dynamic Library Handles That For Service **/
#pragma region Dyncmic Library Manage
    unordered_map<std::string, FSharedLibrary> mCoreLibraryMap;
    unordered_map<std::string, FSharedLibrary> mExtendLibraryMap;
    mutable std::shared_mutex mLibraryMutex;
#pragma endregion

    /** Running Service Map **/
    unordered_map<int64_t, std::shared_ptr<UServiceContext>> mServiceMap;
    mutable std::shared_mutex mServiceMutex;

    /** Service Name To Service ID Mapping **/
    unordered_map<std::string, int64_t> mNameToServiceID;
    mutable std::shared_mutex mNameMutex;

    /** Service ID Set With Same Library Filename **/
    unordered_map<std::string, unordered_set<int64_t>> mFilenameMapping;
    mutable std::shared_mutex mFileNameMutex;

    /** Service ID Management **/
    TIdentAllocator<int64_t, true> mAllocator;
};

