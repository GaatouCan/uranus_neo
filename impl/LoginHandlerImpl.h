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
};
