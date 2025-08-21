#pragma once

#include "base/Types.h"
#include "base/Recycler.h"


class UServer;
class UAgent;
class IPackage_Interface;
class IDataAsset_Interface;
class IEventParam_Interface;

using std::shared_ptr;
using std::unique_ptr;
using std::make_shared;
using std::make_unique;
using FPackageHandle = FRecycleHandle<IPackage_Interface>;


class BASE_API IPlayerBase {

    friend class UServer;

public:
    IPlayerBase();
    virtual ~IPlayerBase();

    [[nodiscard]] int64_t GetPlayerID() const;

    virtual void OnLogin();
    virtual void OnLogout();

    virtual void Save();

    virtual void OnPackage(IPackage_Interface *pkg);
    virtual void OnEvent(IEventParam_Interface *event);

protected:
    virtual void OnReset();
};
