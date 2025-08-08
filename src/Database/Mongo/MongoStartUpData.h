#pragma once

#include "../../Base/DataAsset.h"

#include <string>

class BASE_API FMongoStartUpData final : public IDataAsset_Interface {
public:
    std::string mUri;
    std::string mDatabaseName;
};
