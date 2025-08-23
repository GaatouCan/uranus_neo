#pragma once

#include "AgentBase.h"
#include "factory/PlayerHandle.h"


class IAgentHandler;
class IPlayerBase;
class IServiceBase;
class IPackageCodec_Interface;

using std::unique_ptr;
using std::shared_ptr;
using std::weak_ptr;


enum class EAgentState {
    CREATED,
    INITIALIZED,
    CONNECTED,
    IDLE,
    RUNNING,
    WAITING,
    TERMINATED,
};


class BASE_API UPlayerAgent final : public IAgentBase {

    using APackageChannel = TConcurrentChannel<void(std::error_code, FPackageHandle)>;

public:
    explicit UPlayerAgent(unique_ptr<IPackageCodec_Interface> &&codec);
    ~UPlayerAgent() override;

    [[nodiscard]] ATcpSocket &GetSocket() const;
    [[nodiscard]] bool IsSocketOpen() const;
    [[nodiscard]] asio::ip::address RemoteAddress() const;
    [[nodiscard]] const std::string &GetKey() const;

public:
    [[nodiscard]] int64_t GetPlayerID() const;

    bool Initial(IModuleBase *pModule, IDataAsset_Interface *pData) override;
    void SetExpireSecond(int sec);

    void ConnectToClient();
    void Disconnect();

    void SetUpPlayer(FPlayerHandle &&plr);
    [[nodiscard]] FPlayerHandle ExtractPlayer();

#pragma region Post
    void PostPackage(const FPackageHandle &pkg) const;
    void PostPackage(const std::string &name, const FPackageHandle &pkg) const;

    // TODO
    // void PostTask(int64_t target, const AServiceTask &task) const;
    // void PostTask(const std::string &name, const AServiceTask &task) const;
#pragma endregion

#pragma region Event
    void ListenEvent(int event);
    void RemoveListener(int event) const;
    void DispatchEvent(const shared_ptr<IEventParam_Interface> &param) const;
#pragma endregion

    void SendPackage(const FPackageHandle &pkg);

    void OnLoginFailed(int code, const std::string &desc);
    void OnRepeated(const std::string &addr);

protected:
    [[nodiscard]] IActorBase *GetActor() const override;

private:
    shared_ptr<UPlayerAgent> SharedFromThis();
    weak_ptr<UPlayerAgent> WeakFromThis();

    awaitable<void> WritePackage();
    awaitable<void> ReadPackage();
    awaitable<void> Watchdog();

    void CleanUp() override;

private:
    unique_ptr<IPackageCodec_Interface> mCodec;
    unique_ptr<IAgentHandler> mHandler;

    APackageChannel mOutput;

    ASteadyTimer mWatchdog;
    ASteadyTimePoint mReceiveTime;
    ASteadyDuration mExpiration;

    FPlayerHandle mPlayer;

    std::string mKey;
    bool bCachable;
};
