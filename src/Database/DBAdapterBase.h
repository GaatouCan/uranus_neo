#pragma once

#include "Common.h"

class IDataAsset_Interface;
struct IDBContextBase;
class UDataAccess;

class BASE_API IDBAdapterBase {

    UDataAccess *mOwner;

public:
    IDBAdapterBase();
    virtual ~IDBAdapterBase();

    DISABLE_COPY_MOVE(IDBAdapterBase)

    void SetUpModule(UDataAccess *module);

    [[nodiscard]] UDataAccess *GetDataAccess() const;

    virtual void Initial(const IDataAsset_Interface *data);
    virtual void Stop();

    virtual IDBContextBase *AcquireContext() = 0;
};