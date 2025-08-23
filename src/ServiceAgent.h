#pragma once

#include "AgentBase.h"
#include "base/DataAsset.h"
#include "base/SharedLibrary.h"


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


class BASE_API UTickerNode final : public IChannelNode_Interface {

    ASteadyTimePoint mTickTime;
    ASteadyDuration mDeltaTime;

public:
    UTickerNode();

    void SetCurrentTickTime(ASteadyTimePoint timepoint);
    void SetDeltaTime(ASteadyDuration delta);

    void Execute(IActorBase *pActor) const override;
};


/**
 * The Context Between Services And The Server.
 * Used To Manage Independent Resources Of A Single Service,
 * And Exchange Data Between Inner Service And The Server.
 */
class BASE_API UServiceAgent final : public IAgentBase {

public:
    explicit UServiceAgent(asio::io_context &ctx);
    ~UServiceAgent() override;

    DISABLE_COPY_MOVE(UServiceAgent)

    void SetUpServiceID(int64_t sid);
    void SetUpLibrary(const FSharedLibrary &library);

    [[nodiscard]] int64_t GetServiceID() const;
    [[nodiscard]] std::string GetServiceName() const;

    bool Initial(UServer *pServer, IDataAsset_Interface *pData) override;
    bool BootService();
    void Stop();

    void PushTicker(ASteadyTimePoint timepoint, ASteadyDuration delta);

// #pragma region Post
//     void PostPackage(const FPackageHandle &pkg) const;
//     void PostPackage(const std::string &name, const FPackageHandle &pkg) const;
//
//     void PostTask(int64_t target, const AServiceTask &task) const;
//     void PostTask(const std::string &name, const AServiceTask &task) const;
//
//     void SendToPlayer(int64_t pid, const FPackageHandle &pkg) const;
//     void SendToClient(int64_t pid, const FPackageHandle &pkg) const;
//
//     void PostToPlayer(int64_t pid, const APlayerTask &task) const;
// #pragma endregion

#pragma region Event
    void ListenEvent(int event);
    void RemoveListener(int event) const;
    void DispatchEvent(const shared_ptr<IEventParam_Interface> &param) const;
#pragma endregion

protected:
    [[nodiscard]] IActorBase *GetActor() const override;
    void CleanUp() override;

private:
    shared_ptr<UServiceAgent> SharedFromThis();
    weak_ptr<UServiceAgent> WeakFromThis();

private:
    int64_t mServiceID;
    IServiceBase *mService;

    FSharedLibrary mLibrary;
};
