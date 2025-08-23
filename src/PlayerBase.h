#pragma once

#include "ActorBase.h"

#include <functional>

class UServer;
class UPlayerAgent;
class IPackage_Interface;
class IDataAsset_Interface;
class IEventParam_Interface;


class BASE_API IPlayerBase : public IActorBase {

    friend class UServer;

public:
    IPlayerBase();
    ~IPlayerBase() override;

    [[nodiscard]] int64_t GetPlayerID() const;

    virtual void Initial();

    virtual void OnLogin();
    virtual void OnLogout();

    virtual void Save();

protected:
    virtual void OnReset();
};
