#pragma once

#include "Recycler.h"
#include "PlatformInfo.h"

#include <string>


class IPackage_Interface;
class UPlayerAgent;
class UServer;
using FPackageHandle = FRecycleHandle<IPackage_Interface>;

struct FLogoutRequest {
    int64_t player_id;
};


class BASE_API IAgentHandler {

public:
    IAgentHandler();
    virtual ~IAgentHandler();

    DISABLE_COPY_MOVE(IAgentHandler)

    void SetUpAgent(UPlayerAgent *agent);

    [[nodiscard]] UPlayerAgent* GetAgent() const;
    [[nodiscard]] UServer* GetServer() const;

    virtual FPackageHandle OnLoginSuccess(int64_t pid) = 0;
    virtual FPackageHandle OnLoginFailure(int code, const std::string &desc) = 0;
    virtual FPackageHandle OnRepeated(const std::string &addr) = 0;

    virtual FPlatformInfo ParsePlatformInfo(const FPackageHandle &pkg) = 0;
    virtual FLogoutRequest ParseLogoutRequest(const FPackageHandle &pkg) = 0;

private:
    UPlayerAgent* mAgent;
};
