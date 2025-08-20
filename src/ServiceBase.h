#pragma once

#include "Server.h"
#include "base/Types.h"
#include "base/Package.h"
#include "base/RecycleHandle.h"
#include "base/EventParam.h"


class UContext;
class IDataAsset_Interface;

using std::shared_ptr;
using FPackageHandle = FRecycleHandle<IPackage_Interface>;


enum class EServiceState {
    CREATED,
    INITIALIZED,
    RUNNING,
    TERMINATED,
};

class BASE_API IServiceBase {

    friend class UContext;

protected:
    void SetUpContext(UContext *pContext);
    [[nodiscard]] UContext *GetContext() const;

public:
    IServiceBase();
    virtual ~IServiceBase();

    DISABLE_COPY_MOVE(IServiceBase)

    [[nodiscard]] virtual std::string GetServiceName() const;
    [[nodiscard]] int32_t GetServiceID() const;

    [[nodiscard]] EServiceState GetState() const;

    [[nodiscard]] io_context &GetIOContext() const;
    [[nodiscard]] UServer *GetServer() const;

    template<CModuleType Module>
    Module *GetModule() const;

    [[nodiscard]] FPackageHandle BuildPackage() const;

protected:
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

#pragma region Task
    void PostTask(int64_t target, const std::function<void(IServiceBase *)> &task) const;
    void PostTask(const std::string &name, const std::function<void(IServiceBase *)> &task) const;

    template<class Type, class Callback, class... Args>
    requires std::derived_from<Type, IServiceBase>
    void PostTaskT(int64_t target, Callback &&func, Args &&... args);

    template<class Type, class Callback, class... Args>
    requires std::derived_from<Type, IServiceBase>
    void PostTaskT(const std::string &name, Callback &&func, Args &&... args);
#pragma endregion

#pragma region To Player
    virtual void SendToPlayer(int64_t pid, const FPackageHandle &pkg) const;
    virtual void PostToPlayer(int64_t pid, const std::function<void(IServiceBase *)> &task) const;

    template<class Type, class Callback, class... Args>
    requires std::derived_from<Type, IServiceBase>
    void PostToPlayerT(int64_t pid, Callback &&func, Args &&... args);
#pragma endregion

    virtual void SendToClient(int64_t pid, const FPackageHandle &pkg) const;

#pragma region Event
    virtual void ListenEvent(int event) const;
    virtual void RemoveListener(int event) const;

    void DispatchEvent(const shared_ptr<IEventParam_Interface> &event) const;

    template<CEventType Type, class ... Args>
    void DispatchEventT(Args && ... args) const;
#pragma endregion

#pragma region Timer
    [[nodiscard]] int64_t CreateTimer(const std::function<void(IServiceBase *)> &task, int delay, int rate = -1) const;
    void CancelTimer(int64_t tid) const;
    void CancelAllTimers() const;
#pragma endregion

    void TryCreateLogger(const std::string &name) const;

#pragma region Implenment In Derived Class
    virtual void OnPackage(IPackage_Interface *pkg);
    virtual void OnEvent(IEventParam_Interface *event);
    virtual void OnUpdate(ASteadyTimePoint now, ASteadyDuration delta);
#pragma endregion

protected:
    UContext *mContext;
    std::atomic<EServiceState> mState;

    bool bUpdatePerTick;
};

template<CModuleType Module>
inline Module *IServiceBase::GetModule() const {
    return GetServer()->GetModule<Module>();
}

template<class Type, class Callback, class ... Args> requires std::derived_from<Type, IServiceBase>
inline void IServiceBase::PostTaskT(const int64_t target, Callback &&func, Args &&...args) {
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
        auto *pService = dynamic_cast<Type *>(ser);
        if (pService == nullptr)
            return;

        std::invoke(func, pService, args...);
    };
    this->PostTask(name, task);
}

template<class Type, class Callback, class ... Args> requires std::derived_from<Type, IServiceBase>
inline void IServiceBase::PostToPlayerT(int64_t pid, Callback &&func, Args &&...args) {
    auto task = [func = std::forward<Callback>(func), ...args = std::forward<Args>(args)](IServiceBase *ser) {
        auto *pService = dynamic_cast<Type *>(ser);
        if (pService == nullptr)
            return;

        std::invoke(func, pService, std::forward<Args>(args)...);
    };
    this->PostToPlayer(pid, task);
}

template<CEventType Type, class ... Args>
inline void IServiceBase::DispatchEventT(Args &&...args) const {
    auto event = std::make_shared<Type>(std::forward<Args>(args)...);
    this->DispatchEvent(event);
}
