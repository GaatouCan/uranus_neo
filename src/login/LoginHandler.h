#pragma once

#include "base/Package.h"
#include "base/RecycleHandle.h"
#include "base/PlatformInfo.h"


class UServer;

using FPackageHandle = FRecycleHandle<IPackage_Interface>;


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

    virtual FLoginToken     ParseLoginRequest(const FPackageHandle &pkg) = 0;
    virtual FPlatformInfo   ParsePlatformInfo(const FPackageHandle &pkg) = 0;

protected:
    [[nodiscard]] UServer *GetServer() const;

private:
    ULoginAuth *mLoginAuth;
};
