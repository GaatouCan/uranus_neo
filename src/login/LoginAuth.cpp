#include "LoginAuth.h"
#include "Server.h"
#include "gateway/Gateway.h"
#include "network/Network.h"
#include "network/Connection.h"

#include <spdlog/spdlog.h>


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
        return;
    }

    const auto *network = GetServer()->GetModule<UNetwork>();
    const auto *gateway = GetServer()->GetModule<UGateway>();

    if (network == nullptr || gateway == nullptr) {
        return;
    }

    if (const auto exist = gateway->GetConnectionKey(pid); !exist.empty() && gateway->GetState() == EModuleState::RUNNING) {
        std::string addr = "UNKNOWN";
        if (const auto conn = network->FindConnection(exist)) {
            addr = conn->RemoteAddress().to_string();
        }

        const auto response = network->BuildPackage();

        mLoginHandler->OnRepeatLogin(pid, addr, response);

        response->SetSource(SERVER_SOURCE_ID);
        response->SetTarget(CLIENT_TARGET_ID);

        network->OnLoginFailure(key, response);
    }

    // TODO: Real Login Verify Logic

    // FIXME: Delete This When Finish Real Login Logic
    OnLoginSuccess(key, pid);
}

void ULoginAuth::OnPlatformInfo(const int64_t pid, const FPackageHandle &pkg) const {
    if (mState != EModuleState::RUNNING)
        return;

    auto info = mLoginHandler->ParsePlatformInfo(pkg);
    if (info.operateSystemName.empty())
        return;

    info.playerID = pid;
    if (const auto *gateway = GetServer()->GetModule<UGateway>()) {
        gateway->OnPlatformInfo(info);
    }
}

void ULoginAuth::OnAgentError(const std::string &key, const int64_t pid, const std::string &error) const {
    const auto *network = GetServer()->GetModule<UNetwork>();
    if (network == nullptr)
        return;

    const auto conn = network->FindConnection(key);
    if (conn == nullptr)
        return;

    const auto response = network->BuildPackage();

    mLoginHandler->OnAgentError(pid, conn->RemoteAddress().to_string(), response, error);

    response->SetSource(SERVER_SOURCE_ID);
    response->SetTarget(CLIENT_TARGET_ID);

    network->OnLoginFailure(key, nullptr);
}

void ULoginAuth::OnLoginSuccess(const std::string &key, const int64_t pid) {
    {
        std::unique_lock lock(mMutex);
        mRecentLoginMap.erase(key);
    }

    const auto *network = GetServer()->GetModule<UNetwork>();
    auto *gateway = GetServer()->GetModule<UGateway>();

    if (network == nullptr || gateway == nullptr) {
        return;
    }

    // Send Login Success Response To Client
    const auto response = network->BuildPackage();

    mLoginHandler->OnLoginSuccess(pid, response);

    response->SetSource(SERVER_SOURCE_ID);
    response->SetTarget(CLIENT_TARGET_ID);

    network->OnLoginSuccess(key, pid, response);

    // Create Player Agent In Gateway
    gateway->OnPlayerLogin(pid, key);
}
