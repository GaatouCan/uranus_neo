#pragma once

#include "export.h"
#include "base/AgentHandler.h"


class IMPL_API UAgentHandlerImpl final : public IAgentHandler {
public:
    FPackageHandle OnLoginSuccess(int64_t pid) override;
    FPackageHandle OnLoginFailure(int code, const std::string &desc) override;
    FPackageHandle OnRepeated(const std::string &addr) override;

    FPlatformInfo ParsePlatformInfo(const FPackageHandle &pkg) override;
};
