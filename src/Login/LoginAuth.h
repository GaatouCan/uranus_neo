#pragma once

#include "Module.h"
#include "LoginHandler.h"
#include "Base/Types.h"

#include <unordered_map>
#include <memory>
#include <mutex>
#include <asio.hpp>


class BASE_API ULoginAuth final : public IModuleBase {

    DECLARE_MODULE(ULoginAuth)

protected:
    ULoginAuth();

    void Initial() override;
    void Start() override;
    void Stop() override;

public:
    ~ULoginAuth() override;

    constexpr const char *GetModuleName() const override {
        return "Login Module";
    }

    template<class Type, class ... Args>
    requires std::derived_from<Type, ILoginHandler>
    void SetLoginHandler(Args && ... args);

    bool VerifyAddress(const asio::ip::tcp::endpoint &endpoint);
    void OnPlayerLogin(int64_t cid, const std::shared_ptr<FPacket> &pkg);

private:
    void OnLoginSuccess(int64_t cid, int64_t pid);

private:
    std::unique_ptr<ILoginHandler> mLoginHandler;

    std::unordered_map<int64_t, ASteadyTimePoint> mRecentLoginMap;
    mutable std::mutex mMutex;
};

template<class Type, class ... Args>
requires std::derived_from<Type, ILoginHandler>
inline void ULoginAuth::SetLoginHandler(Args &&...args) {
    if (mState != EModuleState::CREATED)
        return;

    mLoginHandler = std::unique_ptr<ILoginHandler>(new Type(this, std::forward<Args>(args)...));
}
