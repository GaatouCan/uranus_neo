#pragma once

#include "base/PlatformInfo.h"

#include <memory>


class IPackage_Interface;
class UServer;
class ULoginAuth;


class BASE_API ILoginHandler {

    friend class ULoginAuth;

protected:

    struct FLoginToken {
        std::string token;
        int64_t player_id;
    };

    explicit ILoginHandler(ULoginAuth *module);

public:
    ILoginHandler() = delete;
    virtual ~ILoginHandler();

    DISABLE_COPY_MOVE(ILoginHandler)

    virtual void UpdateAddressList() = 0;

    virtual FLoginToken ParseLoginRequest(const std::shared_ptr<IPackage_Interface> &pkg) = 0;
    virtual FPlatformInfo ParsePlatformInfo(const std::shared_ptr<IPackage_Interface> &pkg) = 0;

    virtual void OnLoginSuccess(
        int64_t pid,
        const std::shared_ptr<IPackage_Interface> &pkg) const = 0;

    virtual void OnRepeatLogin(
        int64_t pid,
        const std::string &addr,
        const std::shared_ptr<IPackage_Interface> &pkg) = 0;

    virtual void OnAgentError(
        int64_t pid,
        const std::string &addr,
        const std::shared_ptr<IPackage_Interface> &pkg,
        const std::string &desc) = 0;

protected:
    [[nodiscard]] UServer *GetServer() const;

private:
    ULoginAuth *mLoginAuth;
};
