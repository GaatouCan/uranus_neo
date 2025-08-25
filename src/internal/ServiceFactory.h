#pragma once

#include "factory/ServiceFactory.h"

#include <absl/container/flat_hash_map.h>


class BASE_API UServiceFactory final : public IServiceFactory_Interface {

public:
    void LoadService() override;
    [[nodiscard]] FServiceHandle CreateInstance(const std::string &name) const override;

private:
    absl::flat_hash_map<std::string, FSharedLibrary> mCoreMap;
    absl::flat_hash_map<std::string, FSharedLibrary> mExtendMap;
};
