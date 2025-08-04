#pragma once

#include "DataAsset.h"

#include <string>

class BASE_API FMongoAdapterStartUp final : public IDataAsset_Interface {
public:
    std::string mUri;
};
