#pragma once

#include "ActorBase.h"
#include "timer/TimerHandle.h"

#include <functional>

class UServer;
class UPlayerAgent;
class IServiceBase;
class IPackage_Interface;
class IDataAsset_Interface;
class IEventParam_Interface;

using AActorTask = std::function<void(IActorBase *)>;


/**
 * The Base Class Of Player
 */
class BASE_API IPlayerBase : public IActorBase {

    friend class UPlayerAgent;
    friend class UGateway;

    /** The Player ID **/
    int64_t mPlayerID;

public:
    IPlayerBase();
    ~IPlayerBase() override;

    [[nodiscard]] int64_t GetPlayerID() const;

    virtual void Initial();

    virtual void OnLogin();
    virtual void OnLogout();

    virtual void Save();
    virtual void OnReset();

    /// Send Package To The Client
    void SendPackage(const FPackageHandle &pkg) const;

    /// Send Package To Service With Set Target In Package
    void PostPackage(const FPackageHandle &pkg) const;

    /// Send Package To Service By Service Name
    void PostPackage(const std::string &name, const FPackageHandle &pkg) const;

    /// Post Task To Service
    void PostTask(int64_t target, const AActorTask &task) const;

    /// Post Task To Service By Service Name
    void PostTask(const std::string &name, const AActorTask &task) const;

    template<class Type, class Callback, class... Args>
    requires std::derived_from<Type, IServiceBase>
    void PostTaskT(int64_t target, Callback &&func, Args &&... args);

    template<class Type, class Callback, class... Args>
    requires std::derived_from<Type, IServiceBase>
    void PostTaskT(const std::string &name, Callback &&func, Args &&... args);

    /// Create Timer And Return A Timer Handle
    [[nodiscard]] FTimerHandle CreateTimer(const ATimerTask &task, int delay, int rate = -1) const;

    template<class Target, class Functor, class... Args>
    requires std::derived_from<Target, IPlayerBase>
    FTimerHandle CreateTimer(int delay, int rate, Functor && func, Target *obj, Args &&... args);

    /// Cancel Timer With Timer ID
    void CancelTimer(int64_t tid) const;

    /// Cancel All The Timer About This Service
    void CancelAllTimers() const;

private:
    void SetPlayerID(int64_t id);

    /// Reset The Point To Agent
    void CleanAgent();
};

template<class Type, class Callback, class ... Args> requires std::derived_from<Type, IServiceBase>
inline void IPlayerBase::PostTaskT(const int64_t target, Callback &&func, Args &&...args) {
    auto task = [func = std::forward<Callback>(func), ...args = std::forward<Args>(args)](IActorBase *pActor) {
        if (auto *pService = dynamic_cast<Type *>(pActor)) {
            std::invoke(func, pService, std::forward<Args>(args)...);
        }
    };

    this->PostTask(target, task);
}

template<class Type, class Callback, class ... Args> requires std::derived_from<Type, IServiceBase>
inline void IPlayerBase::PostTaskT(const std::string &name, Callback &&func, Args &&...args) {
    auto task = [func = std::forward<Callback>(func), ...args = std::forward<Args>(args)](IActorBase *pActor) {
        if (auto *pService = dynamic_cast<Type *>(pActor)) {
            std::invoke(func, pService, std::forward<Args>(args)...);
        }
    };

    this->PostTask(name, task);
}

template<class Target, class Functor, class ... Args>
requires std::derived_from<Target, IPlayerBase>
inline FTimerHandle IPlayerBase::CreateTimer(int delay, int rate, Functor &&func, Target *obj, Args &&...args) {
    if (obj == nullptr)
        return {};

    auto task = [func = std::forward<Functor>(func), obj, ...args = std::forward<Args>(args)]
    (ASteadyTimePoint point, ASteadyDuration delta) {
        std::invoke(func, obj, point, delta, std::forward<Args>(args)...);
    };

    return this->CreateTimer(task, delay, rate);
}
