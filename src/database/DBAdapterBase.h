#pragma once

#include "Common.h"

#include <memory>

class IDataAsset_Interface;
struct IDBContext_Interface;
class UDataAccess;
class IDBTaskBase;

class BASE_API IDBAdapterBase {

    UDataAccess *mDataAccess;

public:
    IDBAdapterBase();
    virtual ~IDBAdapterBase();

    DISABLE_COPY_MOVE(IDBAdapterBase)

    void SetUpModule(UDataAccess *module);

    [[nodiscard]] UDataAccess *GetDataAccess() const;

    virtual void Initial(const IDataAsset_Interface *data);
    virtual void Stop();

    virtual IDBContext_Interface *AcquireContext() = 0;

protected:
    void PushTask(std::unique_ptr<IDBTaskBase> &&task) const;
};