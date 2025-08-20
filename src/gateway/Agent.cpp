#include "Agent.h"
#include "Gateway.h"
#include "network/Connection.h"

#include <format>
#include <spdlog/spdlog.h>

UAgent::UAgent()
    : mGateway(nullptr),
      mPlayerID(-1),
      mPlayer(nullptr),
      mState(EAgentState::CREATED) {
}

UAgent::~UAgent() {
}

int64_t UAgent::GetPlayerID() const {
    // TODO
    return 0;
}

UGateway *UAgent::GetGateway() const {
    return mGateway;
}

UServer *UAgent::GetUServer() const {
    if (!mGateway)
        throw std::logic_error(std::format("{} - Gateway Is Null Pointer", __FUNCTION__));

    return mGateway->GetServer();
}

void UAgent::SetUpModule(UGateway *gateway) {
    if (mState != EAgentState::CREATED)
        throw std::logic_error(std::format("{} - Only Called In CREATED State", __FUNCTION__));

    mGateway = gateway;
}

void UAgent::SetUpConnection(const shared_ptr<UConnection> &conn) {
    if (mState != EAgentState::CREATED)
        throw std::logic_error(std::format("{} - Only Called In CREATED State", __FUNCTION__));

    mConn = conn;
}

void UAgent::SetUpPlayerID(const int64_t pid) {
    if (mState != EAgentState::CREATED)
        throw std::logic_error(std::format("{} - Only Called In CREATED State", __FUNCTION__));

    mPlayerID = pid;
    mState = EAgentState::INITIALIZED;
}

void UAgent::Start() {
    if (mState != EAgentState::INITIALIZED)
        throw std::logic_error(std::format("{} - Only Called In INITIALIZED State", __FUNCTION__));

    const auto conn = mConn.lock();
    if (!conn || !conn->IsSocketOpen()) {

        return;
    }

    co_spawn(conn->GetSocket().get_executor(), [self = shared_from_this()]() -> awaitable<void> {
        co_await self->AsyncBoot();
    }, detached);
}

void UAgent::OnRepeat() {
    if (mState >= EAgentState::WAITING)
        return;

    if (mChannel) {
        mChannel->close();
        mChannel.reset();
    }

    // Do Not Need To Remove Self From Gateway
    // TODO: Send The Message To The Old Client And Then Disconnect It

    mPlayerID = -1;
    mConn.reset();

    const bool bImmediate = mState.load() == EAgentState::IDLE;
    mState = EAgentState::WAITING;

    if (const auto conn = mConn.lock(); conn && bImmediate) {
        auto &executor = conn->GetSocket().get_executor();
        co_spawn(executor, [self = shared_from_this()]() -> awaitable<void> {
            self->Shutdown();
            co_return;
        }, detached);
    }
}

void UAgent::OnLogout() {
    if (mState >= EAgentState::WAITING)
        return;

    if (mChannel) {
        mChannel->close();
        mChannel.reset();
    }

    mConn.reset();

    const bool bImmediate = mState.load() == EAgentState::IDLE;
    mState = EAgentState::WAITING;

    if (bImmediate)
        this->Shutdown();
}

awaitable<void> UAgent::AsyncBoot() {
    if (mState != EAgentState::INITIALIZED)
        throw std::logic_error(std::format("{} - Only Called In INITIALIZED State", __FUNCTION__));

    auto executor = co_await asio::this_coro::executor;

    mChannel = make_unique<AAgentChannel>(executor, 1024);
    // TODO: Create Player Instance

    mState = EAgentState::IDLE;
    co_await ProcessChannel();
}

awaitable<void> UAgent::ProcessChannel() {
    if (mState != EAgentState::IDLE)
        throw std::logic_error(std::format("{} - Only Called In IDLE State", __FUNCTION__));

    try {
        while (mState < EAgentState::WAITING && mChannel->is_open()) {
            const auto [ec, node] = co_await mChannel->async_receive();

            if (mState >= EAgentState::WAITING)
                co_return;

            if (ec || node == nullptr)
                continue;

            mState = EAgentState::RUNNING;
            node->Execute(mPlayer);

            if (mState >= EAgentState::WAITING) {
                this->Shutdown();
                co_return;
            }

            mState = EAgentState::IDLE;
        }
    } catch (const std::exception &e) {
        SPDLOG_ERROR("{} - {}", __FUNCTION__, e.what());
    }
}

void UAgent::Shutdown() {
    if (mState == EAgentState::TERMINATED)
        return;

    mState = EAgentState::TERMINATED;

    if (mPlayerID > 0) {
        mGateway->RemoveAgent(mPlayerID);
        mPlayerID = -1;
    }

    // TODO: Player Logout And Destroy It
}
