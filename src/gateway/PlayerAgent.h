#pragma once

#include "ServiceBase.h"


class UGateway;


class BASE_API IPlayerAgent : public IServiceBase {

    using Super = IServiceBase;

public:
    IPlayerAgent();
    ~IPlayerAgent() override;

    [[nodiscard]] std::string GetServiceName() const override;

    [[nodiscard]] int64_t GetPlayerID() const;

    void SendToPlayer(int64_t pid, const shared_ptr<IPackage_Interface> &pkg) const final;
    void PostToPlayer(int64_t pid, const std::function<void(IServiceBase *)> &task) const final;

    void SendToClient(const shared_ptr<IPackage_Interface> &pkg) const;

    virtual void OnHeartBeat(const shared_ptr<IPackage_Interface> &pkg);

private:
    void SendToClient(int64_t pid, const shared_ptr<IPackage_Interface> &pkg) const final;
};
