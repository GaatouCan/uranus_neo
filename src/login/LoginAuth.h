#pragma once

#include "Module.h"
#include "LoginHandler.h"
#include "base/Types.h"

#include <absl/container/flat_hash_map.h>


class BASE_API ULoginAuth final : public IModuleBase {

    DECLARE_MODULE(ULoginAuth)

protected:
    void Initial() override;
    void Start() override;
    void Stop() override;

public:
    ULoginAuth();
    ~ULoginAuth() override;

    constexpr const char *GetModuleName() const override {
        return "Login Module";
    }

    template<class Type, class ... Args>
    requires std::derived_from<Type, ILoginHandler>
    void SetLoginHandler(Args && ... args);

    bool VerifyAddress(const asio::ip::tcp::endpoint &endpoint);

    void OnLoginRequest(const std::string &key, const FPackageHandle &pkg);
    FPlatformInfo OnPlatformInfo(int64_t pid, const FPackageHandle &pkg) const;

private:
    void OnLoginSuccess(const std::string &key, int64_t pid);

private:
    std::unique_ptr<ILoginHandler> mLoginHandler;

    absl::flat_hash_map<std::string, ASteadyTimePoint> mRecentLoginMap;
    mutable std::mutex mMutex;
};

template<class Type, class ... Args>
requires std::derived_from<Type, ILoginHandler>
inline void ULoginAuth::SetLoginHandler(Args &&...args) {
    if (mState != EModuleState::CREATED)
        return;

    mLoginHandler = std::unique_ptr<ILoginHandler>(new Type(this, std::forward<Args>(args)...));
}
