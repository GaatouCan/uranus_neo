#pragma once

#include "ServiceBase.h"


class UGateway;
class UAgentContext;


class BASE_API IPlayerAgent : public IServiceBase {

    friend class UAgentContext;
    using Super = IServiceBase;

public:
    IPlayerAgent();
    ~IPlayerAgent() override;

    [[nodiscard]] std::string GetServiceName() const override;

    [[nodiscard]] int64_t GetPlayerID() const;

    void SendToPlayer(int64_t pid, const FPackageHandle &pkg) const final;
    void PostToPlayer(int64_t pid, const std::function<void(IServiceBase *)> &task) const final;

    void SendToClient(const FPackageHandle &pkg) const;

protected:
    virtual void OnHeartBeat(const FPackageHandle &pkg);

private:
    void SendToClient(int64_t pid, const FPackageHandle &pkg) const final;
};
