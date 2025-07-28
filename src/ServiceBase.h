#pragma once

#include "Base/TimerHandle.h"

#include <string>
#include <memory>
#include <asio.hpp>


class UServer;
class FPackage;
class IContextBase;
class IEventParam_Interface;
class IDataAsset_Interface;


enum class BASE_API EServiceState {
    CREATED,
    INITIALIZED,
    RUNNING,
    TERMINATED,
};

class BASE_API IServiceBase {

    friend class IContextBase;

protected:
    void SetUpContext(IContextBase *context);
    [[nodiscard]] IContextBase *GetContext() const;

public:
    IServiceBase();
    virtual ~IServiceBase();

    DISABLE_COPY_MOVE(IServiceBase)

    [[nodiscard]] virtual std::string GetServiceName() const;
    [[nodiscard]] int32_t GetServiceID() const;

    [[nodiscard]] asio::io_context &GetIOContext() const;
    [[nodiscard]] UServer *GetServer() const;

    [[nodiscard]] EServiceState GetState() const;

    virtual bool Initial(const IDataAsset_Interface *data);
    virtual bool Start();
    virtual void Stop();

    [[nodiscard]] std::shared_ptr<FPackage> BuildPackage() const;

    virtual void OnPackage(const std::shared_ptr<FPackage> &pkg);
    virtual void OnEvent(const std::shared_ptr<IEventParam_Interface> &event);

#pragma region Package
    /// Send To Other Service Use Target In Package
    void PostPackage(const std::shared_ptr<FPackage> &pkg) const;

    /// Send To Other Service Use Service Name
    void PostPackage(const std::string &name, const std::shared_ptr<FPackage> &pkg) const;
#pragma endregion

#pragma region Task
    void PostTask(int32_t target, const std::function<void(IServiceBase *)> &task) const;
    void PostTask(const std::string &name, const std::function<void(IServiceBase *)> &task) const;

    template<class Type, class Callback, class... Args>
    requires std::derived_from<Type, IServiceBase>
    void PostTaskT(int32_t target, Callback &&func, Args &&... args);

    template<class Type, class Callback, class... Args>
    requires std::derived_from<Type, IServiceBase>
    void PostTaskT(const std::string &name, Callback &&func, Args &&... args);
#pragma endregion

#pragma region To Player
    virtual void SendToPlayer(int64_t pid, const std::shared_ptr<FPackage> &pkg) const;
    virtual void PostToPlayer(int64_t pid, const std::function<void(IServiceBase *)> &task) const;

    template<class Type, class Callback, class... Args>
    requires std::derived_from<Type, IServiceBase>
    void PostToPlayerT(int64_t pid, Callback &&func, Args &&... args);
#pragma endregion

    virtual void SendToClient(int64_t pid, const std::shared_ptr<FPackage> &pkg) const;

#pragma region Event
    virtual void ListenEvent(int event) const;
    virtual void RemoveListener(int event) const;

    void DispatchEvent(const std::shared_ptr<IEventParam_Interface> &event) const;
#pragma endregion

#pragma region Timer
    virtual FTimerHandle SetSteadyTimer(const std::function<void(IServiceBase *)> &task, int delay, int rate) const;
    virtual FTimerHandle SetSystemTimer(const std::function<void(IServiceBase *)> &task, int delay, int rate) const;
    virtual void CancelTimer(const FTimerHandle &handle);
#pragma endregion

protected:
    IContextBase *mContext;
    std::atomic<EServiceState> mState;
};

template<class Type, class Callback, class ... Args> requires std::derived_from<Type, IServiceBase>
inline void IServiceBase::PostTaskT(int32_t target, Callback &&func, Args &&...args) {
    auto task = [func = std::forward<Callback>(func), ...args = std::forward<Args>(args)](IServiceBase *ser) {
        auto *ptr = dynamic_cast<Type *>(ser);
        if (ptr == nullptr)
            return;

        std::invoke(func, ptr, args...);
    };
    this->PostTask(target, task);
}

template<class Type, class Callback, class ... Args> requires std::derived_from<Type, IServiceBase>
inline void IServiceBase::PostTaskT(const std::string &name, Callback &&func, Args &&...args) {
    auto task = [func = std::forward<Callback>(func), ...args = std::forward<Args>(args)](IServiceBase *ser) {
        auto *ptr = dynamic_cast<Type *>(ser);
        if (ptr == nullptr)
            return;

        std::invoke(func, ptr, args...);
    };
    this->PostTask(name, task);
}

template<class Type, class Callback, class ... Args> requires std::derived_from<Type, IServiceBase>
inline void IServiceBase::PostToPlayerT(int64_t pid, Callback &&func, Args &&...args) {
    auto task = [func = std::forward<Callback>(func), ...args = std::forward<Args>(args)](IServiceBase *ser) {
        auto *ptr = dynamic_cast<Type *>(ser);
        if (ptr == nullptr)
            return;

        std::invoke(func, ptr, args...);
    };
    this->PostToPlayer(pid, task);
}
