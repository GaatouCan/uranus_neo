#pragma once

#include "export.h"

#include <login/LoginHandler.h>


class IMPL_API ULoginHandlerImpl final : public ILoginHandler {

public:
    explicit ULoginHandlerImpl(ULoginAuth *module);
    ~ULoginHandlerImpl() override;

    void UpdateAddressList() override;
    FLoginToken ParseLoginRequest(const std::shared_ptr<IPackage_Interface> &pkg) override;

    void OnLoginSuccess(int64_t pid, const std::shared_ptr<IPackage_Interface> &pkg) const override;
    void OnRepeatLogin(int64_t pid, const std::string &addr, const std::shared_ptr<IPackage_Interface> &pkg) override;
};
