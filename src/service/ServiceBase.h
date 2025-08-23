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

class BASE_API IServiceBase : public IActorBase {

    friend class UServiceAgent;

public:
    IServiceBase();
    ~IServiceBase() override;

    DISABLE_COPY_MOVE(IServiceBase)

    [[nodiscard]] virtual std::string GetServiceName() const;
    [[nodiscard]] int64_t GetServiceID() const;

    [[nodiscard]] EServiceState GetState() const;

    [[nodiscard]] FPackageHandle BuildPackage() const;

    virtual bool Initial(const IDataAsset_Interface *pData);
    virtual awaitable<bool> AsyncInitial(const IDataAsset_Interface *pData);

    virtual bool Start();
    virtual void Stop();

    /// Send To Other Service Use Target In Package
    void PostPackage(const FPackageHandle &pkg) const;

    /// Send To Other Service Use Service Name
    void PostPackage(const std::string &name, const FPackageHandle &pkg) const;

    void PostTask(int64_t target, const AActorTask &task) const;
    void PostTask(const std::string &name, const AActorTask &task) const;

    template<class Type, class Callback, class... Args>
    requires std::derived_from<Type, IServiceBase>
    void PostTaskT(int64_t target, Callback &&func, Args &&... args);

    template<class Type, class Callback, class... Args>
    requires std::derived_from<Type, IServiceBase>
    void PostTaskT(const std::string &name, Callback &&func, Args &&... args);

    void SendToPlayer(int64_t pid, const FPackageHandle &pkg) const;
    void PostToPlayer(int64_t pid, const AActorTask &task) const;

    template<class Type, class Callback, class... Args>
    requires std::derived_from<Type, IPlayerBase>
    void PostToPlayerT(int64_t pid, Callback &&func, Args &&... args);

    void SendToClient(int64_t pid, const FPackageHandle &pkg) const;

    virtual void ListenEvent(int event) const;
    virtual void RemoveListener(int event) const;

    void DispatchEvent(const shared_ptr<IEventParam_Interface> &event) const;

    [[nodiscard]] FTimerHandle CreateTimer(const ATimerTask &task, int delay, int rate = -1) const;
    void CancelTimer(int64_t tid) const;
    void CancelAllTimers() const;

    void TryCreateLogger(const std::string &name) const;

    void OnPackage(IPackage_Interface *pkg) override;
    void OnEvent(IEventParam_Interface *event) override;
    virtual void OnUpdate(ASteadyTimePoint now, ASteadyDuration delta);

protected:
    std::atomic<EServiceState> mState;
    bool bUpdatePerTick;
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
