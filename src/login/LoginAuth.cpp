#include "LoginAuth.h"
#include "Server.h"
#include "base/Package.h"
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

void ULoginAuth::OnPlayerLogin(const int64_t cid, const std::shared_ptr<IPackage_Interface> &pkg) {
    if (mState != EModuleState::RUNNING)
        return;

    {
        std::unique_lock lock(mMutex);
        if (const auto iter = mRecentLoginMap.find(cid); iter != mRecentLoginMap.end()) {
            if (std::chrono::steady_clock::now() - iter->second < std::chrono::seconds(1))
                return;
        }

        mRecentLoginMap[cid] = std::chrono::steady_clock::now();
    }

    const auto [token, pid] = mLoginHandler->ParseLoginRequest(pkg);
    if (token.empty() || pid == 0) {
        SPDLOG_WARN("{:<20} - fd[{}] Parse Login Request Failed.", __FUNCTION__, cid);
        return;
    }

    const auto *network = GetServer()->GetModule<UNetwork>();
    const auto *gateway = GetServer()->GetModule<UGateway>();

    if (network == nullptr || gateway == nullptr) {
        return;
    }

    if (const auto exist = gateway->GetConnectionID(pid); exist != 0 && gateway->GetState() == EModuleState::RUNNING) {
        std::string addr = "UNKNOWN";
        if (const auto conn = network->FindConnection(exist)) {
            addr = conn->RemoteAddress().to_string();
        }

        const auto response = network->BuildPackage();

        mLoginHandler->OnRepeatLogin(pid, addr, response);

        response->SetSource(SERVER_SOURCE_ID);
        response->SetTarget(CLIENT_TARGET_ID);

        network->OnLoginFailure(cid, response);
    }

    // TODO: Real Login Verify Logic

    // FIXME: Delete This When Finish Real Login Logic
    OnLoginSuccess(cid, pid);
}

void ULoginAuth::OnAgentError(const int64_t cid, int64_t pid, const std::string &error) const {
    if (const auto *network = GetServer()->GetModule<UNetwork>()) {
        // FIXME: Send A Message
        network->OnLoginFailure(cid, nullptr);
    }
}

void ULoginAuth::OnLoginSuccess(const int64_t cid, const int64_t pid) {
    {
        std::unique_lock lock(mMutex);
        mRecentLoginMap.erase(cid);
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

    network->OnLoginSuccess(cid, pid, response);

    // Create Player Agent In Gateway
    gateway->OnPlayerLogin(pid, cid);
}
