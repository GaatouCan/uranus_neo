#pragma once

#include "Recycler.h"

#include <string>


class IPackage_Interface;
class UAgent;
class UServer;
using FPackageHandle = FRecycleHandle<IPackage_Interface>;

class BASE_API IAgentHandler {

public:
    IAgentHandler();
    virtual ~IAgentHandler();

    DISABLE_COPY_MOVE(IAgentHandler)

    void SetUpAgent(UAgent *agent);

    [[nodiscard]] UAgent* GetAgent() const;
    [[nodiscard]] UServer* GetServer() const;

    virtual FPackageHandle OnLoginSuccess(int64_t pid) = 0;
    virtual FPackageHandle OnLoginFailure(const std::string &desc) = 0;

    virtual FPackageHandle OnRepeated(const std::string &addr) = 0;

private:
    UAgent* mAgent;
};
