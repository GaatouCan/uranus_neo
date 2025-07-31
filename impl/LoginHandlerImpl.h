#pragma once

#include "export.h"
#include <Login/LoginHandler.h>


class IMPL_API ULoginHandlerImpl final : public ILoginHandler {

public:
    explicit ULoginHandlerImpl(ULoginAuth *module);
    ~ULoginHandlerImpl() override;

    void UpdateAddressList() override;
    FLoginToken ParseLoginRequest(const std::shared_ptr<FPacket> &pkg) override;

    void OnLoginSuccess(int64_t pid, const std::shared_ptr<FPacket> &pkg) const override;
    void OnRepeatLogin(int64_t pid, const std::string &addr, const std::shared_ptr<FPacket> &pkg) override;
};
