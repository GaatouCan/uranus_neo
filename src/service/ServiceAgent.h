#pragma once

#include "AgentBase.h"
#include "base/DataAsset.h"
#include "base/SharedLibrary.h"


class IServiceBase;

/**
 * Wrapper Of Update Data
 */
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
 * The Agent To Run A Single Service.
 * Used To Manage Independent Resources Of A Single Service,
 * And Exchange Data Between The Inner Service And The Server.
 */
class BASE_API UServiceAgent final : public IAgentBase {

    /** The Service ID Allocate By Service Module **/
    int64_t mServiceID;

    /** The Inner Service Instance **/
    IServiceBase *mService;

    /** The Shared Library Of Creator And Destroyer **/
    FSharedLibrary mLibrary;

public:
    explicit UServiceAgent(asio::io_context &ctx);
    ~UServiceAgent() override;

    DISABLE_COPY_MOVE(UServiceAgent)

    /// Set Up The Service ID
    void SetUpServiceID(int64_t sid);

    /// Set Up The Shared Library To Create And Destroy The Inner Service
    void SetUpLibrary(const FSharedLibrary &library);

    /// Return The Service ID
    [[nodiscard]] int64_t GetServiceID() const;

    /// Return The Service Name After Service Initial
    [[nodiscard]] std::string GetServiceName() const;

    /// Initial The Service With DataAsset
    bool Initial(IModuleBase *pModule, IDataAsset_Interface *pData) override;

    /// Boot The Service
    bool BootService();

    /// Close The Channel And The Service Will Be Stopped In ::CleanUp
    void Stop();

    /// Push Tick Data To The Inner Channel
    void PushTicker(ASteadyTimePoint timepoint, ASteadyDuration delta);

    /// Post Package To Service, Set The Target In The Package
    void PostPackage(const FPackageHandle &pkg) const;

    /// Post The Package To Service By Service's Name
    void PostPackage(const std::string &name, const FPackageHandle &pkg) const;

    /// Post Task To Service
    void PostTask(int64_t target, const AActorTask &task) const;

    /// Post Task To Service By Service's Name
    void PostTask(const std::string &name, const AActorTask &task) const;

    /// Send The Package To The Player
    void SendToPlayer(int64_t pid, const FPackageHandle &pkg) const;

    /// Send The Package To The Client
    void SendToClient(int64_t pid, const FPackageHandle &pkg) const;

    /// Post Task To Player
    void PostToPlayer(int64_t pid, const AActorTask &task) const;

    /// Register Self To Event Module With Specific Event Type
    void ListenEvent(int event);

    /// Will Not Listen Specific Event Type Any More
    void RemoveListener(int event) const;

    /// Dispatch Event
    void DispatchEvent(const shared_ptr<IEventParam_Interface> &param) const;

protected:
    [[nodiscard]] IActorBase *GetActor() const override;

    void CleanUp() override;

private:
    /// Get Shared Pointer Helper
    shared_ptr<UServiceAgent> SharedFromThis();

    /// Get Weak Pointer Helper
    weak_ptr<UServiceAgent> WeakFromThis();
};
