#include "LoginAuth.h"
#include "Server.h"

#include <spdlog/spdlog.h>

#include "Agent.h"


ULoginAuth::ULoginAuth() {
}

ULoginAuth::~ULoginAuth() {
    Stop();
}

void ULoginAuth::Initial() {
    if (mState != EModuleState::CREATED)
        return;

    mState = EModuleState::INITIALIZED;
}

void ULoginAuth::Start() {
    if (mState != EModuleState::INITIALIZED)
        return;

    mState = EModuleState::RUNNING;
}

void ULoginAuth::Stop() {
    if (mState == EModuleState::STOPPED)
        return;

    assert(mLoginHandler != nullptr);
    mState = EModuleState::STOPPED;
}

bool ULoginAuth::VerifyAddress(const asio::ip::tcp::endpoint &endpoint) {
    // TODO
    return true;
}

void ULoginAuth::OnLoginRequest(const std::string &key, const FPackageHandle &pkg) {
    if (mState != EModuleState::RUNNING)
        return;

    {
        std::unique_lock lock(mMutex);
        if (const auto iter = mRecentLoginMap.find(key); iter != mRecentLoginMap.end()) {
            if (std::chrono::steady_clock::now() - iter->second < std::chrono::seconds(1))
                return;
        }

        mRecentLoginMap[key] = std::chrono::steady_clock::now();
    }

    const auto [token, pid] = mLoginHandler->ParseLoginRequest(pkg);
    if (token.empty() || pid == 0) {
        SPDLOG_WARN("{:<20} - fd[{}] Parse Login Request Failed.", __FUNCTION__, key);

        if (const auto agent = GetServer()->FindAgent(key)) {
            agent->OnLoginFailed(1, "Fail To Parse Token");
        }

        return;
    }

    // TODO: Real Login Verify Logic

    // FIXME: Delete This When Finish Real Login Logic
    OnLoginSuccess(key, pid);
}

FPlatformInfo ULoginAuth::OnPlatformInfo(const int64_t pid, const FPackageHandle &pkg) const {
    if (mState != EModuleState::RUNNING)
        return {};

    auto info = mLoginHandler->ParsePlatformInfo(pkg);
    if (info.operateSystemName.empty())
        return {};

    info.playerID = pid;
    return info;
}

void ULoginAuth::OnLoginSuccess(const std::string &key, const int64_t pid) {
    {
        std::unique_lock lock(mMutex);
        mRecentLoginMap.erase(key);
    }

    GetServer()->OnPlayerLogin(key, pid);
}
