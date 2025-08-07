#pragma once

#include "ContextBase.h"


class UGateway;


class BASE_API UAgentContext final : public IContextBase {

    int64_t mPlayerID;
    int64_t mConnectionID;

public:
    UAgentContext();
    ~UAgentContext() override;

    void SetPlayerID(int64_t pid);
    void SetConnectionID(int64_t cid);

    [[nodiscard]] int64_t GetPlayerID() const;
    [[nodiscard]] int64_t GetConnectionID() const;

    [[nodiscard]] int32_t GetServiceID() const override;

    [[nodiscard]] UGateway* GetGateway() const;
    [[nodiscard]] std::shared_ptr<UAgentContext> GetOtherAgent(int64_t pid) const;

    void OnHeartBeat(const std::shared_ptr<IPackage_Interface> &pkg) const;
};
