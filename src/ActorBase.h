#pragma once

#include "base/Recycler.h"
#include "base/Types.h"


class IPackage_Interface;
class IEventParam_Interface;
class UServer;

using FPackageHandle = FRecycleHandle<IPackage_Interface>;
using ATimerTask = std::function<void(ASteadyTimePoint, ASteadyDuration)>;


class BASE_API IActorBase {

public:
    IActorBase() = default;
    virtual ~IActorBase() = default;

    virtual void OnPackage(IPackage_Interface *pkg);
    virtual void OnEvent(IEventParam_Interface *event);

    // [[nodiscard]] asio::io_context &GetIOContext() const;
};
