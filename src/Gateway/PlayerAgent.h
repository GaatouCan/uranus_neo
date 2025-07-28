#pragma once

#include "Service/Service.h"
#include "Context.h"


class IPlayerAgent;
class UGateway;


class BASE_API UAgentContext final : public IContextBase {

    int64_t mPlayerID;
    int64_t mConnectionID;

public:
    UAgentContext();
    ~UAgentContext() override = default;

    [[nodiscard]] int32_t GetServiceID() const override;

    void SetPlayerID(int64_t pid);
    [[nodiscard]] int64_t GetPlayerID() const;

    void SetConnectionID(int64_t cid);
    [[nodiscard]] int64_t GetConnectionID() const;

    void OnHeartBeat(const std::shared_ptr<IPackageInterface> &pkg) const;

    UGateway *GetGateway() const;
};


class BASE_API IPlayerAgent : public IServiceBase {

    using Super = IServiceBase;

public:
    IPlayerAgent();
    ~IPlayerAgent() override;

    [[nodiscard]] std::string GetServiceName() const override;

    [[nodiscard]] int64_t GetPlayerID() const;

    void SendToPlayer(int64_t pid, const std::shared_ptr<IPackageInterface> &pkg) const final;
    void PostToPlayer(int64_t pid, const std::function<void(IServiceBase *)> &task) const final;

    void SendToClient(const std::shared_ptr<IPackageInterface> &pkg) const;

    FTimerHandle SetSteadyTimer(const std::function<void(IServiceBase *)> &task, int delay, int rate) const final;
    FTimerHandle SetSystemTimer(const std::function<void(IServiceBase *)> &task, int delay, int rate) const final;
    void CancelTimer(const FTimerHandle &handle) final;

    virtual void OnHeartBeat(const std::shared_ptr<IPackageInterface> &pkg);

    void ListenEvent(int event) const final;
    void RemoveListener(int event) const final;

private:
    void SendToClient(int64_t pid, const std::shared_ptr<IPackageInterface> &pkg) const final;
};
