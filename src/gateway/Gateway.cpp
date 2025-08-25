#include "Gateway.h"
#include "Server.h"
#include "PlayerAgent.h"
#include "PlayerBase.h"
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

    // Find The Not Login Agent By Key And Check If Login Agent With Same Player ID Existed
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

    // Agent Not Exist
    if (!agent)
        return;

    // Query The Cached Map
    // If There Is Player Instance With This Player ID
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

    // Login Repeated
    if (existed) {
        existed->OnRepeated(agent->RemoteAddress().to_string());
    }

    // Reset The Old Instance
    if (player) {
        player->Save();
        player->OnReset();
    } else {
        // Create New One
        player = mPlayerFactory->CreatePlayer();
    }

    // Set The Player ID Whether Is New Or Cached
    player->SetPlayerID(pid);

    // Move The Player To The Agent
    // The Lifetime Of The Player Is Managed By The Agent From Now On
    agent->SetUpPlayer(std::move(player));
}

void UGateway::ForeachPlayer(const std::function<bool(const shared_ptr<UPlayerAgent> &)> &func) const {
    if (mState != EModuleState::RUNNING)
        return;

    std::set<shared_ptr<UPlayerAgent>> players;

    {
        std::shared_lock lock(mPlayerMutex);
        for (const auto &player : mPlayerMap | std::views::values) {
            players.insert(player);
        }
    }

    for (const auto &plr : players) {
        if (std::invoke(func, plr))
            return;
    }
}

void UGateway::Initial() {
    if (mState != EModuleState::CREATED)
        throw std::logic_error(std::format("{} - Module[{}] Not In CREATED State", __FUNCTION__, GetModuleName()));

    mAcceptor = make_unique<ATcpAcceptor>(GetIOContext());
    mCacheTimer = make_unique<ASteadyTimer>(GetIOContext());

    // Load The Library
    mPlayerFactory->Initial();

    // Run The IO Context Pool
    mIOContextPool.Start(4);

    mState = EModuleState::INITIALIZED;
}

void UGateway::Start() {
    if (mState != EModuleState::INITIALIZED)
        throw std::logic_error(std::format("{} - Module[{}] Not In INITIALIZED State", __FUNCTION__, GetModuleName()));

    const auto &cfg = GetServer()->GetServerConfig();
    const auto port = cfg["server"]["port"].as<uint16_t>();

    // Begin To Waiting Client Connect
    co_spawn(GetIOContext(), WaitForClient(port), detached);

    // Start The Cache Collect Coroutine
    co_spawn(GetIOContext(), CollectCachedPlayer(), detached);

    mState = EModuleState::RUNNING;
}

void UGateway::Stop() {
    if (mState == EModuleState::STOPPED)
        return;

    mState = EModuleState::STOPPED;

    mCacheTimer->cancel();
    mIOContextPool.Stop();
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
                // Check If The IP Address In Blacklist
                if (auto *login = GetServer()->GetModule<ULoginAuth>(); login != nullptr) {
                    if (!login->VerifyAddress(socket.remote_endpoint())) {
                        SPDLOG_WARN("Reject Client From {}", socket.remote_endpoint().address().to_string());
                        socket.close();
                        continue;
                    }
                }

                // Create New Codec
                auto codec = GetServer()->CreateUniquePackageCodec(std::move(socket));

                // Create New Player Agent
                const auto agent = make_shared<UPlayerAgent>(std::move(codec));
                const auto key = agent->GetKey();

                if (key.empty())
                    continue;

                // Check If The Key Repeated
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

                // Run The Agent And Waiting The Login Request
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
