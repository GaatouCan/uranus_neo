#pragma once

#include "Package.h"
#include "RecycleHandle.h"

#include <string>

using FPackageHandle = FRecycleHandle<IPackage_Interface>;
class UAgent;
class UServer;

class BASE_API IAgentHandler {

public:
    IAgentHandler();
    virtual ~IAgentHandler();

    DISABLE_COPY_MOVE(IAgentHandler)

    void SetUpAgent(UAgent *agent);

    [[nodiscard]] UAgent* GetAgent() const;
    [[nodiscard]] UServer* GetServer() const;

    virtual FPackageHandle OnLoginSuccess(int64_t pid) = 0;
    virtual FPackageHandle OnLoginFailure(int64_t pid, const std::string &desc) = 0;

    virtual FPackageHandle OnRepeated(const std::string &addr) = 0;

private:
    UAgent* mAgent;
};
