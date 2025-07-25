#pragma once

#include "Common.h"

class BASE_API IDataAsset_Interface {

public:
    virtual ~IDataAsset_Interface() = default;

    virtual void SyncLoad() {}
};
