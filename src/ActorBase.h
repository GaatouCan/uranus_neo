#pragma once

#include "base/Recycler.h"
#include "base/Types.h"


class IAgentBase;
class IPackage_Interface;
class IEventParam_Interface;
class UServer;

using FPackageHandle = FRecycleHandle<IPackage_Interface>;
using ATimerTask = std::function<void(ASteadyTimePoint, ASteadyDuration)>;

/**
 * The Basic Class Of Player And Service
 */
class BASE_API IActorBase {

public:
    IActorBase();
    virtual ~IActorBase();

    void SetUpAgent(IAgentBase *agent);
    [[nodiscard]] IAgentBase *GetAgent() const;

    /// Implement This To Handle The Package Data
    virtual void OnPackage(IPackage_Interface *pkg);

    /// Implement This To Handle The Event Param
    virtual void OnEvent(IEventParam_Interface *event);

    [[nodiscard]] asio::io_context &GetIOContext() const;
    [[nodiscard]] UServer *GetServer() const;

protected:
    template<class T>
    T *GetAgentT() const {
        auto res = dynamic_cast<T *>(GetAgent());

        if (res == nullptr)
            throw std::bad_cast();

        return res;
    }

protected:
    /** The Pointer To The Agent **/
    IAgentBase *mAgent;
};
