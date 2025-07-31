#pragma once

#include "Common.h"

class UServer;

class BASE_API IDataAsset_Interface {

public:
    virtual ~IDataAsset_Interface() = default;

    virtual void SetUpServer(UServer *server);
    virtual void LoadSynchronous() {}
};
