#pragma once

#include "ActorBase.h"

#include <functional>

class UServer;
class UPlayerAgent;
class IServiceBase;
class IPackage_Interface;
class IDataAsset_Interface;
class IEventParam_Interface;

using AActorTask = std::function<void(IActorBase *)>;


class BASE_API IPlayerBase : public IActorBase {

    friend class UPlayerAgent;
    friend class UGateway;

public:
    IPlayerBase();
    ~IPlayerBase() override;

    [[nodiscard]] int64_t GetPlayerID() const;

    virtual void Initial();

    virtual void OnLogin();
    virtual void OnLogout();

    virtual void Save();
    virtual void OnReset();

    void SendPackage(const FPackageHandle &pkg) const;

    void PostPackage(const FPackageHandle &pkg) const;
    void PostPackage(const std::string &name, const FPackageHandle &pkg) const;

    void PostTask(int64_t target, const AActorTask &task) const;
    void PostTask(const std::string &name, const AActorTask &task) const;

    template<class Type, class Callback, class... Args>
    requires std::derived_from<Type, IServiceBase>
    void PostTaskT(int64_t target, Callback &&func, Args &&... args);

    template<class Type, class Callback, class... Args>
    requires std::derived_from<Type, IServiceBase>
    void PostTaskT(const std::string &name, Callback &&func, Args &&... args);

private:
    void SetPlayerID(int64_t id);
    void CleanAgent();

private:
    int64_t mPlayerID;
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
