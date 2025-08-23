#pragma once

#include "ActorBase.h"
#include "timer/TimerHandle.h"

#include <string>

class UServer;
class IActorBase;
class IPlayerBase;
class IPackage_Interface;
class IDataAsset_Interface;
class IEventParam_Interface;

using std::shared_ptr;
using std::unique_ptr;
using std::make_shared;
using std::make_unique;
using AActorTask = std::function<void(IActorBase *)>;


enum class EServiceState {
    CREATED,
    INITIALIZED,
    RUNNING,
    TERMINATED,
};

/**
 * The Basic Class Of Service
 */
class BASE_API IServiceBase : public IActorBase {

    friend class UServiceAgent;

protected:
    /** Current Service State **/
    std::atomic<EServiceState> mState;

    /** If It Update Per Tick **/
    bool bUpdatePerTick;

public:
    IServiceBase();
    ~IServiceBase() override;

    DISABLE_COPY_MOVE(IServiceBase)

    /// It Must Be A Unique And Valid Name After Initial
    [[nodiscard]] virtual std::string GetServiceName() const;

    /// Get The Service ID
    [[nodiscard]] int64_t GetServiceID() const;

    /// Return The Current State Of Service
    [[nodiscard]] EServiceState GetState() const;

    /// Return A Package Handle From Agent
    [[nodiscard]] FPackageHandle BuildPackage() const;

    virtual bool Initial(const IDataAsset_Interface *pData);
    virtual awaitable<bool> AsyncInitial(const IDataAsset_Interface *pData);

    virtual bool Start();
    virtual void Stop();

    /// Send To Other Service Use Target In Package
    void PostPackage(const FPackageHandle &pkg) const;

    /// Send To Other Service Use Service Name
    void PostPackage(const std::string &name, const FPackageHandle &pkg) const;

    /// Post Task To Other Service
    void PostTask(int64_t target, const AActorTask &task) const;

    /// Post Task To Other Service By Other's Name
    void PostTask(const std::string &name, const AActorTask &task) const;

    template<class Type, class Callback, class... Args>
    requires std::derived_from<Type, IServiceBase>
    void PostTaskT(int64_t target, Callback &&func, Args &&... args);

    template<class Type, class Callback, class... Args>
    requires std::derived_from<Type, IServiceBase>
    void PostTaskT(const std::string &name, Callback &&func, Args &&... args);

    /// Send Package To Player
    void SendToPlayer(int64_t pid, const FPackageHandle &pkg) const;

    /// Post Task To Player
    void PostToPlayer(int64_t pid, const AActorTask &task) const;

    template<class Type, class Callback, class... Args>
    requires std::derived_from<Type, IPlayerBase>
    void PostToPlayerT(int64_t pid, Callback &&func, Args &&... args);

    /// Send Package To The Client
    void SendToClient(int64_t pid, const FPackageHandle &pkg) const;

    /// Listen Event With The Specific Event Type
    void ListenEvent(int event) const;

    /// Do Not Listen The Specific Event Type Any More
    void RemoveListener(int event) const;

    /// Dispatch Event
    void DispatchEvent(const shared_ptr<IEventParam_Interface> &event) const;

    /// Create Timer And Return A Timer Handle
    [[nodiscard]] FTimerHandle CreateTimer(const ATimerTask &task, int delay, int rate = -1) const;

    template<class Target, class Functor, class... Args>
    requires std::derived_from<Target, IServiceBase>
    FTimerHandle CreateTimer(int delay, int rate, Functor && func, Target *obj, Args &&... args);

    /// Cancel Timer With Timer ID
    void CancelTimer(int64_t tid) const;

    /// Cancel All The Timer About This Service
    void CancelAllTimers() const;

    void TryCreateLogger(const std::string &name) const;
    
    /// Implement By Derived Class
    virtual void OnUpdate(ASteadyTimePoint now, ASteadyDuration delta);
};


template<class Type, class Callback, class ... Args>
requires std::derived_from<Type, IServiceBase>
inline void IServiceBase::PostTaskT(const int64_t target, Callback &&func, Args &&...args) {
    auto task = [func = std::forward<Callback>(func), ...args = std::forward<Args>(args)](IActorBase *pActor) {
        if (auto *pService = dynamic_cast<Type *>(pActor)) {
            std::invoke(func, pService, std::forward<Args>(args)...);
        }
    };

    this->PostTask(target, task);
}

template<class Type, class Callback, class ... Args>
requires std::derived_from<Type, IServiceBase>
inline void IServiceBase::PostTaskT(const std::string &name, Callback &&func, Args &&...args) {
    auto task = [func = std::forward<Callback>(func), ...args = std::forward<Args>(args)](IActorBase *pActor) {
        if (auto *pService = dynamic_cast<Type *>(pActor)) {
            std::invoke(func, pService, std::forward<Args>(args)...);
        }
    };

    this->PostTask(name, task);
}

template<class Type, class Callback, class ... Args>
requires std::derived_from<Type, IPlayerBase>
inline void IServiceBase::PostToPlayerT(const int64_t pid, Callback &&func, Args &&...args) {
    auto task = [func = std::forward<Callback>(func), ...args = std::forward<Args>(args)](IActorBase *pAcotr) {
        if (auto *pPlayer = dynamic_cast<Type *>(pAcotr)) {
            std::invoke(func, pPlayer, std::forward<Args>(args)...);
        }
    };
    this->PostToPlayer(pid, task);
}

template<class Target, class Functor, class ... Args>
requires std::derived_from<Target, IServiceBase>
FTimerHandle IServiceBase::CreateTimer(int delay, int rate, Functor &&func, Target *obj, Args &&...args) {
    if (obj == nullptr)
        return {};

    auto task = [func = std::forward<Functor>(func), obj, ...args = std::forward<Args>(args)]
    (ASteadyTimePoint point, ASteadyDuration delta) {
        std::invoke(func, obj, point, delta, std::forward<Args>(args)...);
    };

    return this->CreateTimer(task, delay, rate);
}
