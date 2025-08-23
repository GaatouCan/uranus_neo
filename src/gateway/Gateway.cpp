#include "Gateway.h"
#include "Server.h"
#include "PlayerAgent.h"
#include "base/AgentHandler.h"
#include "base/PackageCodec.h"
#include "login/LoginAuth.h"

#include <spdlog/spdlog.h>


UGateway::UGateway() {
}

UGateway::~UGateway() {
    Stop();
}

unique_ptr<IAgentHandler> UGateway::CreateAgentHandler() const {
    if (mPlayerFactory == nullptr)
        throw std::runtime_error(fmt::format("{} - PlayerFactory Is Null", __FUNCTION__));

    return mPlayerFactory->CreateAgentHandler();
}

shared_ptr<UPlayerAgent> UGateway::FindPlayer(const int64_t pid) const {
    if (mState != EModuleState::RUNNING)
        return nullptr;

    std::shared_lock lock(mPlayerMutex);
    const auto iter = mPlayerMap.find(pid);
    return iter == mPlayerMap.end() ? nullptr : iter->second;
}

shared_ptr<UPlayerAgent> UGateway::FindAgent(const std::string &key) const {
    if (mState != EModuleState::RUNNING)
        return nullptr;

    std::shared_lock lock(mAgentMutex);
    const auto iter = mAgentMap.find(key);
    return iter == mAgentMap.end() ? nullptr : iter->second;
}

std::vector<shared_ptr<UPlayerAgent>> UGateway::GetPlayerList(const std::vector<int64_t> &list) const {
    if (mState != EModuleState::RUNNING)
        return {};

    std::vector<shared_ptr<UPlayerAgent>> result;
    std::shared_lock lock(mPlayerMutex);
    for (const auto &pid : list) {
        if (const auto iter = mPlayerMap.find(pid); iter != mPlayerMap.end()) {
            result.push_back(iter->second);
        }
    }

    return result;
}

void UGateway::RemovePlayer(const int64_t pid) {
    if (mState != EModuleState::RUNNING)
        return;

    std::unique_lock lock(mPlayerMutex);
    mPlayerMap.erase(pid);
}

void UGateway::RecyclePlayer(FPlayerHandle &&player) {
    if (mState != EModuleState::RUNNING)
        return;

    if (player == nullptr)
        return;

    const auto pid = player->GetPlayerID();
    if (pid <= 0)
        return;

    this->RemovePlayer(pid);

    std::unique_lock lock(mCacheMutex);
    mCachedMap.insert_or_assign(pid, FCachedNode{
        std::move(player),
        std::chrono::steady_clock::now()
    });
}

void UGateway::RemoveAgent(const std::string &key) {
    if (mState != EModuleState::RUNNING)
        return;

    std::unique_lock lock(mAgentMutex);
    mAgentMap.erase(key);
}

void UGateway::OnPlayerLogin(const std::string &key, const int64_t pid) {
    if (mState != EModuleState::RUNNING)
        return;

    FPlayerHandle player;

    shared_ptr<UPlayerAgent> agent;
    shared_ptr<UPlayerAgent> existed;

    {
        std::scoped_lock lock(mAgentMutex, mPlayerMutex);
        if (const auto iter = mPlayerMap.find(pid); iter != mPlayerMap.end()) {
            if (iter->second->GetKey() == key)
                return;

            player = std::move(iter->second->ExtractPlayer());
            existed = iter->second;
            mPlayerMap.erase(iter);
        }

        if (const auto iter = mAgentMap.find(key); iter != mAgentMap.end()) {
            agent = iter->second;
            mAgentMap.erase(iter);
            mPlayerMap.insert_or_assign(pid, agent);
        }
    }

    // Query The Cached Map
    {
        std::unique_lock lock(mCacheMutex);
        if (player == nullptr) {
            if (const auto iter = mCachedMap.find(pid); iter != mCachedMap.end()) {
                player = std::move(iter->second.player);
                mCachedMap.erase(iter);
            }
        }

        mCachedMap.erase(pid);
    }

    if (!agent)
        return;

    if (existed) {
        existed->OnRepeated(agent->RemoteAddress().to_string());
    }

    if (player) {
        player->Save();
        player->OnReset();
    } else {
        player = mPlayerFactory->CreatePlayer();
    }

    agent->SetUpPlayer(std::move(player));
}

void UGateway::Initial() {
    if (mState != EModuleState::CREATED)
        throw std::logic_error(std::format("{} - Module[{}] Not In CREATED State", __FUNCTION__, GetModuleName()));

    mAcceptor = make_unique<ATcpAcceptor>(GetIOContext());
    mCacheTimer = make_unique<ASteadyTimer>(GetIOContext());

    // Load The Library
    mPlayerFactory->Initial();

    mState = EModuleState::INITIALIZED;
}

void UGateway::Start() {
    if (mState != EModuleState::INITIALIZED)
        throw std::logic_error(std::format("{} - Module[{}] Not In INITIALIZED State", __FUNCTION__, GetModuleName()));

    const auto &cfg = GetServer()->GetServerConfig();

    const auto port = cfg["server"]["port"].as<uint16_t>();

    co_spawn(GetIOContext(), WaitForClient(port), detached);
    co_spawn(GetIOContext(), CollectCachedPlayer(), detached);

    mState = EModuleState::RUNNING;
}

void UGateway::Stop() {
    if (mState == EModuleState::STOPPED)
        return;

    mState = EModuleState::STOPPED;
}

awaitable<void> UGateway::WaitForClient(const uint16_t port) {
    try {
        mAcceptor->open(asio::ip::tcp::v4());
        mAcceptor->bind({asio::ip::tcp::v4(), port});
        mAcceptor->listen(port);

        SPDLOG_INFO("Waiting For Client To Connect - Server Port: {}", port);

        while (mState == EModuleState::RUNNING) {
            auto [ec, socket] = co_await mAcceptor->async_accept(mIOContextPool.GetIOContext());

            if (ec) {
                SPDLOG_ERROR("{:<20} - {}", __FUNCTION__, ec.message());
                continue;
            }

            if (socket.is_open()) {
                if (auto *login = GetServer()->GetModule<ULoginAuth>(); login != nullptr) {
                    if (!login->VerifyAddress(socket.remote_endpoint())) {
                        SPDLOG_WARN("Reject Client From {}", socket.remote_endpoint().address().to_string());
                        socket.close();
                        continue;
                    }
                }

                auto codec = GetServer()->CreateUniquePackageCodec(std::move(socket));

                const auto agent = make_shared<UPlayerAgent>(std::move(codec));
                const auto key = agent->GetKey();

                if (key.empty())
                    continue;

                {
                    std::unique_lock lock(mAgentMutex);
                    if (mAgentMap.contains(key)) {
                        SPDLOG_WARN("{:<20} - Connection[{}] Has Already Exist.", __FUNCTION__, key);
                        continue;
                    }
                    mAgentMap.insert_or_assign(key, agent);
                }

                SPDLOG_INFO("{:<20} - New Connection From {} - key[{}]",
                    __FUNCTION__, agent->RemoteAddress().to_string(), agent->GetKey());

                agent->Initial(this, nullptr);
                agent->ConnectToClient();
            }
        }
    } catch (const std::exception &e) {
        SPDLOG_ERROR("{} - {}", __FUNCTION__, e.what());
    }
}

awaitable<void> UGateway::CollectCachedPlayer() {
    try {
        const auto &cfg = GetServer()->GetServerConfig();

        const auto gap = cfg["server"]["cache"]["collect_gap"].as<int>();
        const auto keepSec = cfg["server"]["cache"]["keep_alive"].as<int>();
        const auto maxSize = cfg["server"]["cache"]["max_size"].as<int>();

        const auto duration = std::chrono::seconds(gap);
        auto point = std::chrono::steady_clock::now();

        while (mState != EModuleState::STOPPED) {
            point += duration;
            mCacheTimer->expires_at(point);
            if (auto [ec] = co_await mCacheTimer->async_wait(); ec) {
                SPDLOG_ERROR("{} - {}", __FUNCTION__, ec.message());
                break;
            }

            std::unique_lock lock(mCacheMutex);
            absl::erase_if(mCachedMap, [point, keepSec](const decltype(mCachedMap)::value_type &pair) {
                return point - pair.second.timepoint > std::chrono::seconds(keepSec);
            });

            if (mCachedMap.size() > maxSize) {
                size_t del = mCachedMap.size() - maxSize;
                for (auto iter = mCachedMap.begin(); iter != mCachedMap.end() && del > 0;) {
                    mCachedMap.erase(iter++);
                    --del;
                }
            }
        }
    } catch (std::exception &e) {
        SPDLOG_ERROR("{} - {}", __FUNCTION__, e.what());
    }
}
