#pragma once

#include "ServiceBase.h"


class UGateway;
class FPackage;


class BASE_API IPlayerAgent : public IServiceBase {

    using Super = IServiceBase;

public:
    IPlayerAgent();
    ~IPlayerAgent() override;

    [[nodiscard]] std::string GetServiceName() const override;

    [[nodiscard]] int64_t GetPlayerID() const;

    void SendToPlayer(int64_t pid, const std::shared_ptr<FPackage> &pkg) const final;
    void PostToPlayer(int64_t pid, const std::function<void(IServiceBase *)> &task) const final;

    void SendToClient(const std::shared_ptr<FPackage> &pkg) const;

    FTimerHandle SetSteadyTimer(const std::function<void(IServiceBase *)> &task, int delay, int rate) const final;
    FTimerHandle SetSystemTimer(const std::function<void(IServiceBase *)> &task, int delay, int rate) const final;
    void CancelTimer(const FTimerHandle &handle) final;

    virtual void OnHeartBeat(const std::shared_ptr<FPackage> &pkg);

    void ListenEvent(int event) const final;
    void RemoveListener(int event) const final;

private:
    void SendToClient(int64_t pid, const std::shared_ptr<FPackage> &pkg) const final;
};
