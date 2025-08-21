#pragma once

#include "base/Types.h"
#include "base/Recycler.h"
#include "base/SharedLibrary.h"
#include "base/ContextHandle.h"


class UServer;
class IServiceBase;
class IPlayerBase;
class IModuleBase;
class IRecyclerBase;
class IPackage_Interface;
class IEventParam_Interface;
class IDataAsset_Interface;

using FPackageHandle = FRecycleHandle<IPackage_Interface>;
using APlayerTask = std::function<void(IPlayerBase *)>;
using AServiceTask = std::function<void(IServiceBase *)>;
using std::unique_ptr;

/**
 * The Context Between Services And The Server.
 * Used To Manage Independent Resources Of A Single Service,
 * And Exchange Data Between Inner Service And The Server.
 */
class BASE_API UContext final : public std::enable_shared_from_this<UContext> {
#pragma region Schedule Wrapper

    /**
     * The Abstract Interface Of The Wrapper Of The Schedulable Things In Context Channel.
     * Used To Wrap The Data Which Send To Inner Service
     */
    class BASE_API ISchedule_Interface {
    public:
        ISchedule_Interface() = default;
        virtual ~ISchedule_Interface() = default;

        DISABLE_COPY_MOVE(ISchedule_Interface)

        /** Call The Service Method To Handle Data **/
        virtual void Execute(IServiceBase *pService) = 0;
    };

    /**
     * The Wrapper Of Package
     */
    class BASE_API UPackageNode final : public ISchedule_Interface {
        FPackageHandle mPackage;

    public:
        void SetPackage(const FPackageHandle &pkg);
        void Execute(IServiceBase *pService) override;
    };


    /**
     * The Wrapper Of Task
     */
    class BASE_API UTaskNode final : public ISchedule_Interface {
        std::function<void(IServiceBase *)> mTask;

    public:
        void SetTask(const std::function<void(IServiceBase *)> &task);
        void Execute(IServiceBase *pService) override;
    };


    /**
     * The Wrapper Of Event
     */
    class BASE_API UEventNode final : public ISchedule_Interface {
        shared_ptr<IEventParam_Interface> mEvent;

    public:
        void SetEventParam(const shared_ptr<IEventParam_Interface> &event);
        void Execute(IServiceBase *pService) override;
    };

    /**
     * The Wrapper Of Ticker
     */
    class BASE_API UTickerNode final : public ISchedule_Interface {
        ASteadyTimePoint mTickTime;
        ASteadyDuration mDeltaTime;

    public:
        UTickerNode();

        void SetCurrentTickTime(ASteadyTimePoint timepoint);
        void SetDeltaTime(ASteadyDuration delta);
        void Execute(IServiceBase *pService) override;
    };
#pragma endregion

    using AContextChannel = TConcurrentChannel<void(std::error_code, unique_ptr<ISchedule_Interface>)>;

public:
    UContext() = delete;

    explicit UContext(asio::io_context &ctx);
    ~UContext();

    DISABLE_COPY_MOVE(UContext)

    void SetUpServer(UServer *pServer);
    void SetUpServiceID(FServiceHandle sid);
    void SetUpLibrary(const FSharedLibrary &library);

    [[nodiscard]] UServer *GetServer() const;
    [[nodiscard]] asio::io_context &GetIOContext() const;

    [[nodiscard]] FServiceHandle GetServiceID() const;
    [[nodiscard]] std::string GetServiceName() const;

    /** Generate A Handle To Represent Itself **/
    [[nodiscard]] FContextHandle GenerateHandle();

    bool Initial(const IDataAsset_Interface *pData);
    bool BootService();
    void Stop();

    [[nodiscard]] FPackageHandle BuildPackage() const;

#pragma region Push
    void PushPackage(const FPackageHandle &pkg);
    void PushTask(const AServiceTask &task);
    void PushEvent(const shared_ptr<IEventParam_Interface> &event);
    void PushTicker(ASteadyTimePoint timepoint, ASteadyDuration delta);
#pragma endregion

#pragma region Post
    void PostPackage(const FPackageHandle &pkg) const;
    void PostPackage(const std::string &name, const FPackageHandle &pkg) const;

    void PostTask(int64_t target, const AServiceTask &task) const;
    void PostTask(const std::string &name, const AServiceTask &task) const;

    void SendToPlayer(int64_t pid, const FPackageHandle &pkg) const;
    void SendToClient(int64_t pid, const FPackageHandle &pkg) const;

    void PostToPlayer(int64_t pid, const APlayerTask &task) const;
#pragma endregion

#pragma region Timer
    int64_t CreateTimer(const std::function<void(IServiceBase *)> &task, int delay, int rate = -1);
    void CancelTimer(int64_t tid) const;
    void CancelAllTimers();
#pragma endregion
//
// #pragma region Event
//     void ListenEvent(int event);
//     void RemoveListener(int event);
//     void DispatchEvent(const shared_ptr<IEventParam_Interface> &param) const;
// #pragma endregion

private:
    awaitable<void> ProcessChannel();
    void CleanUp();

private:
    asio::io_context &mCtx;
    UServer *mServer;

    FServiceHandle mServiceID;
    IServiceBase *mService;

    AContextChannel mChannel;
    unique_ptr<IRecyclerBase> mPool;

    FSharedLibrary mLibrary;
};
