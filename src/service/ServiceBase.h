#pragma once

#include "ActorBase.h"
#include "timer/TimerHandle.h"

#include <string>

class UServer;
class UServiceAgent;
class IPlayerBase;
class IPackage_Interface;
class IDataAsset_Interface;
class IEventParam_Interface;

using std::shared_ptr;
using std::unique_ptr;
using std::make_shared;
using std::make_unique;


enum class EServiceState {
    CREATED,
    INITIALIZED,
    RUNNING,
    TERMINATED,
};

class BASE_API IServiceBase : public IActorBase {

    friend class UServiceAgent;

protected:
    void SetUpContext(UServiceAgent *pContext);
    [[nodiscard]] UServiceAgent *GetContext() const;

public:
    IServiceBase();
    ~IServiceBase() override;

    DISABLE_COPY_MOVE(IServiceBase)

    [[nodiscard]] virtual std::string GetServiceName() const;
    [[nodiscard]] int64_t GetServiceID() const;

    [[nodiscard]] EServiceState GetState() const;

    // [[nodiscard]] asio::io_context &GetIOContext() const;
    [[nodiscard]] UServer *GetServer() const;

    [[nodiscard]] FPackageHandle BuildPackage() const;

#pragma region Control By Context
    virtual bool Initial(const IDataAsset_Interface *pData);
    virtual awaitable<bool> AsyncInitial(const IDataAsset_Interface *pData);

    virtual bool Start();
    virtual void Stop();
#pragma endregion

#pragma region Package
    /// Send To Other Service Use Target In Package
    void PostPackage(const FPackageHandle &pkg) const;

    /// Send To Other Service Use Service Name
    void PostPackage(const std::string &name, const FPackageHandle &pkg) const;
#pragma endregion

// #pragma region Task
//     void PostTask(int64_t target, const std::function<void(IServiceBase *)> &task) const;
//     void PostTask(const std::string &name, const std::function<void(IServiceBase *)> &task) const;
//
//     template<class Type, class Callback, class... Args>
//     requires std::derived_from<Type, IServiceBase>
//     void PostTaskT(int64_t target, Callback &&func, Args &&... args);
//
//     template<class Type, class Callback, class... Args>
//     requires std::derived_from<Type, IServiceBase>
//     void PostTaskT(const std::string &name, Callback &&func, Args &&... args);
// #pragma endregion

#pragma region To Player
    // void SendToPlayer(int64_t pid, const FPackageHandle &pkg) const;
    // void PostToPlayer(int64_t pid, const APlayerTask &task) const;
    //
    // template<class Type, class Callback, class... Args>
    // requires std::derived_from<Type, IPlayerBase>
    // void PostToPlayerT(int64_t pid, Callback &&func, Args &&... args);
#pragma endregion

    void SendToClient(int64_t pid, const FPackageHandle &pkg) const;

#pragma region Event
    virtual void ListenEvent(int event) const;
    virtual void RemoveListener(int event) const;

    void DispatchEvent(const shared_ptr<IEventParam_Interface> &event) const;
#pragma endregion

#pragma region Timer
    [[nodiscard]] FTimerHandle CreateTimer(const ATimerTask &task, int delay, int rate = -1) const;
    void CancelTimer(int64_t tid) const;
    void CancelAllTimers() const;
#pragma endregion

    void TryCreateLogger(const std::string &name) const;

#pragma region Implenment In Derived Class
    void OnPackage(IPackage_Interface *pkg) override;
    void OnEvent(IEventParam_Interface *event) override;
    virtual void OnUpdate(ASteadyTimePoint now, ASteadyDuration delta);
#pragma endregion

protected:
    UServiceAgent *mContext;
    std::atomic<EServiceState> mState;

    bool bUpdatePerTick;
};


// template<class Type, class Callback, class ... Args>
// requires std::derived_from<Type, IServiceBase>
// inline void IServiceBase::PostTaskT(const int64_t target, Callback &&func, Args &&...args) {
//     auto task = [func = std::forward<Callback>(func), ...args = std::forward<Args>(args)](IServiceBase *ser) {
//         auto *ptr = dynamic_cast<Type *>(ser);
//         if (ptr == nullptr)
//             return;
//
//         std::invoke(func, ptr, args...);
//     };
//     this->PostTask(target, task);
// }
//
// template<class Type, class Callback, class ... Args>
// requires std::derived_from<Type, IServiceBase>
// inline void IServiceBase::PostTaskT(const std::string &name, Callback &&func, Args &&...args) {
//     auto task = [func = std::forward<Callback>(func), ...args = std::forward<Args>(args)](IServiceBase *ser) {
//         auto *pService = dynamic_cast<Type *>(ser);
//         if (pService == nullptr)
//             return;
//
//         std::invoke(func, pService, args...);
//     };
//     this->PostTask(name, task);
// }

// template<class Type, class Callback, class ... Args>
// requires std::derived_from<Type, IPlayerBase>
// void IServiceBase::PostToPlayerT(int64_t pid, Callback &&func, Args &&...args) {
//     auto task = [func = std::forward<Callback>(func), ...args = std::forward<Args>(args)](IPlayerBase *plr) {
//         auto *pPlayer = dynamic_cast<Type *>(plr);
//         if (pPlayer == nullptr)
//             return;
//
//         std::invoke(func, pPlayer, args...);
//     };
//     this->PostToPlayer(pid, task);
// }
