#pragma once

#include "ContextBase.h"
#include "base/PlatformInfo.h"


class UGateway;


class BASE_API UAgentContext final : public UContextBase {

    int64_t mPlayerID;
    std::string mConnKey;

public:
    UAgentContext();
    ~UAgentContext() override;

    void SetPlayerID(int64_t pid);
    void SetConnectionKey(const std::string &key);

    [[nodiscard]] int64_t GetPlayerID() const;
    [[nodiscard]] const std::string &GetConnectionKey() const;

    // [[nodiscard]] int32_t GetServiceID() const override;

    [[nodiscard]] UGateway* GetGateway() const;
    [[nodiscard]] shared_ptr<UAgentContext> GetOtherAgent(int64_t pid) const;

    void OnHeartBeat(const shared_ptr<IPackage_Interface> &pkg) const;
    void OnPlatformInfo(const FPlatformInfo &info) const;
};
