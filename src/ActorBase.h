#pragma once

#include "Common.h"


class IPackage_Interface;
class IEventParam_Interface;


class BASE_API IActorBase {

public:
    IActorBase() = default;
    virtual ~IActorBase() = default;

    virtual void OnPackage(IPackage_Interface *pkg);
    virtual void OnEvent(IEventParam_Interface *event);
};
