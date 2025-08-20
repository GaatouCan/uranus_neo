#pragma once

#include "Common.h"

#include <cstdint>

class IPackage_Interface;
class IEventParam_Interface;

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
