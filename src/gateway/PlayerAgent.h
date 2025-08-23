#pragma once

#include "AgentBase.h"
#include "factory/PlayerHandle.h"


class IAgentHandler;
class IPlayerBase;
class IPackageCodec_Interface;


/**
 * The Agent To Run A Player Instance,
 * With The Socket To The Client
 */
class BASE_API UPlayerAgent final : public IAgentBase {

    using APackageChannel = TConcurrentChannel<void(std::error_code, FPackageHandle)>;

    /** The Socket Inside, Use To Read/Write Package Data **/
    unique_ptr<IPackageCodec_Interface> mCodec;

    /** Handle Some Protocol Without Player Instance **/
    unique_ptr<IAgentHandler> mHandler;

    /** The Channel To Send The Package **/
    APackageChannel mOutput;

    /** Watchdog Timer **/
    ASteadyTimer mWatchdog;

    /** Record The Last Receive Package Time Point **/
    ASteadyTimePoint mReceiveTime;

    /** Watchdog Expiration **/
    ASteadyDuration mExpiration;

    /** The Inner Player Instance **/
    FPlayerHandle mPlayer;

    /** The Unique Key To The Socket, Use Before Player Login **/
    std::string mKey;

    /** Enable After Socket Closed, Player Instance Can Be Recycled Back To The UGateway **/
    bool bCachable;

public:
    explicit UPlayerAgent(unique_ptr<IPackageCodec_Interface> &&codec);
    ~UPlayerAgent() override;

    /// Get The Reference Of The Raw Tcp Socket
    [[nodiscard]] ATcpSocket &GetSocket() const;

    /// Return If The Socket Is Running
    [[nodiscard]] bool IsSocketOpen() const;

    /// Return The Client Address
    [[nodiscard]] asio::ip::address RemoteAddress() const;

    /// Return The Socket Key
    [[nodiscard]] const std::string &GetKey() const;

    /// Set The Watchdog Expiration
    void SetExpireSecond(int sec);

    bool Initial(IModuleBase *pModule, IDataAsset_Interface *pData) override;

    /// Connect To Client And Wait Login Request
    void ConnectToClient();

    /// Disconnect From Client And Close The Channel, Prepare To Clean Up
    void Disconnect();

    /// Set The Player Instance To This Agent
    void SetUpPlayer(FPlayerHandle &&plr);

    /// Get The Player ID After Player Login
    [[nodiscard]] int64_t GetPlayerID() const;

    /// Move The Player Instance From This Agent
    [[nodiscard]] FPlayerHandle ExtractPlayer();

    /// Send To Service, With Set Target In Package
    void PostPackage(const FPackageHandle &pkg) const;

    /// Send To Service By Service Name
    void PostPackage(const std::string &name, const FPackageHandle &pkg) const;

    /// Post Task To Service
    void PostTask(int64_t target, const AActorTask &task) const;

    /// Post Task To Service By Service Name
    void PostTask(const std::string &name, const AActorTask &task) const;

    /// Register Self To Event Module With Specific Event Type
    void ListenEvent(int event);

    /// Will Not Listen Specific Event Type Any More
    void RemoveListener(int event) const;

    /// Dispatch Event
    void DispatchEvent(const shared_ptr<IEventParam_Interface> &param) const;

    /// Send Package To Client
    void SendPackage(const FPackageHandle &pkg);

    /// Handle Fail To Login
    void OnLoginFailed(int code, const std::string &desc);

    /// Handle Login From Other Client
    void OnRepeated(const std::string &addr);

protected:
    [[nodiscard]] IActorBase *GetActor() const override;

private:
    /// Get Shared Pointer Helper
    shared_ptr<UPlayerAgent> SharedFromThis();

    /// Get Weak Pointer Helper
    weak_ptr<UPlayerAgent> WeakFromThis();

    /// Write Package Looping
    awaitable<void> WritePackage();

    /// Read Package Looping
    awaitable<void> ReadPackage();

    /// Watchdog Looping
    awaitable<void> Watchdog();

    void CleanUp() override;
};
