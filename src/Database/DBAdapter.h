#pragma once

#include "Common.h"

class IDataAsset_Interface;

class BASE_API IDBAdapter_Interface {

public:
    IDBAdapter_Interface();
    virtual ~IDBAdapter_Interface();

    DISABLE_COPY_MOVE(IDBAdapter_Interface)

    virtual void Initial(const IDataAsset_Interface *data);
    virtual void Stop();
};