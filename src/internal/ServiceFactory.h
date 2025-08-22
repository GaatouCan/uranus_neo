#pragma once

#include "factory/ServiceFactory.h"
#include "base/SharedLibrary.h"

#include <absl/container/flat_hash_map.h>


class BASE_API UServiceFactory final : public IServiceFactory_Interface {

public:
    void LoadService() override;
    [[nodiscard]] FSharedLibrary FindService(const std::string &name) const override;

private:
    absl::flat_hash_map<std::string, FSharedLibrary> mCoreMap;
    absl::flat_hash_map<std::string, FSharedLibrary> mExtendMap;
};
