#pragma once

#include "factory/ServiceFactory.h"
#include "base/SharedLibrary.h"

#include <absl/container/flat_hash_map.h>


class BASE_API UServiceFactory final : public IServiceFactory_Interface {

    using AServiceCreator = IServiceBase *(*)();
    using AServiceDestroyer = void (*)(IServiceBase *);

    struct BASE_API FLibraryNode {
        FSharedLibrary library;
        AServiceCreator creator;
        AServiceDestroyer destroyer;
    };

public:
    void LoadService() override;

    [[nodiscard]] FServiceHandle CreateInstance(const std::string &path) override;
    void DestroyInstance(IServiceBase *pService, const std::string &path) override;

private:
    absl::flat_hash_map<std::string, FLibraryNode> mCoreMap;
    absl::flat_hash_map<std::string, FLibraryNode> mExtendMap;
};
