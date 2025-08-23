#pragma once

#include "Module.h"
#include "base/MultiIOContextPool.h"
#include "factory/PlayerFactory.h"
#include "base/Types.h"

#include <absl/container/flat_hash_map.h>
#include <shared_mutex>



class BASE_API UGateway final : public IModuleBase {

    DECLARE_MODULE(UGateway)

    struct BASE_API FCachedNode {
        FPlayerHandle player;
        ASteadyTimePoint timepoint;
    };

public:
    UGateway();
    ~UGateway() override;

    [[nodiscard]] constexpr const char *GetModuleName() const override {
        return "Gateway";
    }

    template<class T, class... Args>
    requires std::derived_from<T, IPlayerFactory_Interface>
    void SetPlayerFactory(Args && ... args) {
        if (mState != EModuleState::CREATED)
            throw std::logic_error("Only can set PlayerFactory in CREATED state");

        mPlayerFactory = std::make_unique<T>(std::forward<Args>(args)...);
    }

    [[nodiscard]] shared_ptr<UPlayerAgent> FindPlayer(int64_t pid) const;
    [[nodiscard]] shared_ptr<UPlayerAgent> FindAgent(const std::string &key) const;

    [[nodiscard]] std::vector<shared_ptr<UPlayerAgent>> GetPlayerList(const std::vector<int64_t> &list) const;

    void RemovePlayer(int64_t pid);
    void RecyclePlayer(FPlayerHandle &&player);

    void RemoveAgent(const std::string &key);

    void OnPlayerLogin(const std::string &key, int64_t pid);

protected:
    void Initial() override;
    void Start() override;
    void Stop() override;

private:
    awaitable<void> WaitForClient(uint16_t port);
    awaitable<void> CollectCachedPlayer();

private:
    UMultiIOContextPool mIOContextPool;

    unique_ptr<ATcpAcceptor> mAcceptor;
    unique_ptr<ASteadyTimer> mCacheTimer;
    unique_ptr<IPlayerFactory_Interface> mPlayerFactory;

    absl::flat_hash_map<std::string, shared_ptr<UPlayerAgent>> mAgentMap;
    mutable std::shared_mutex mAgentMutex;

    absl::flat_hash_map<int64_t, shared_ptr<UPlayerAgent>> mPlayerMap;
    mutable std::shared_mutex mPlayerMutex;

    absl::flat_hash_map<int64_t, FCachedNode> mCachedMap;
    mutable std::shared_mutex mCacheMutex;
};
