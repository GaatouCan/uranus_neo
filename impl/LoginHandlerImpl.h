#pragma once

#include "export.h"

#include <login/LoginHandler.h>

class IMPL_API ULoginHandlerImpl final : public ILoginHandler {
public:
    explicit ULoginHandlerImpl(ULoginAuth *module);
    ~ULoginHandlerImpl() override;

    void UpdateAddressList() override;

    FLoginToken ParseLoginRequest(const FPackageHandle &pkg) override;
    FPlatformInfo ParsePlatformInfo(const FPackageHandle &pkg) override;

    void OnLoginSuccess(
        int64_t pid,
        const FPackageHandle &pkg) const override;

    void OnRepeatLogin(
        int64_t pid,
        const std::string &addr,
        const FPackageHandle &pkg) override;

    void OnAgentError(
        int64_t pid,
        const std::string &addr,
        const FPackageHandle &pkg,
        const std::string &desc) override;
};
