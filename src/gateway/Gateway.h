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

    /** The IOContext For Sockets **/
    UMultiIOContextPool mIOContextPool;

    /** The Acceptor Use The Main IOContext Is UServer **/
    unique_ptr<ATcpAcceptor> mAcceptor;

    /** The Timer To Collect The Cached Player **/
    unique_ptr<ASteadyTimer> mCacheTimer;

    /** Create The Player Instance And Agent Handler **/
    unique_ptr<IPlayerFactory_Interface> mPlayerFactory;

    /** The All Agent That Without Player Instance(Not Login) **/
    absl::flat_hash_map<std::string, shared_ptr<UPlayerAgent>> mAgentMap;
    mutable std::shared_mutex mAgentMutex;

    /** The All Agent That With Player Instance(Has Login) **/
    absl::flat_hash_map<int64_t, shared_ptr<UPlayerAgent>> mPlayerMap;
    mutable std::shared_mutex mPlayerMutex;

    /** The Cached Player Instance **/
    absl::flat_hash_map<int64_t, FCachedNode> mCachedMap;
    mutable std::shared_mutex mCacheMutex;

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

    /// Create The Agent Handler
    unique_ptr<IAgentHandler> CreateAgentHandler() const;

    /// Find The Login Player Agent By Player ID
    [[nodiscard]] shared_ptr<UPlayerAgent> FindPlayer(int64_t pid) const;

    /// Find The Not Login Agent By Key
    [[nodiscard]] shared_ptr<UPlayerAgent> FindAgent(const std::string &key) const;

    [[nodiscard]] std::vector<shared_ptr<UPlayerAgent>> GetPlayerList(const std::vector<int64_t> &list) const;

    /// Remove The Login Player Agent
    void RemovePlayer(int64_t pid);

    /// Recycle The Player Instance
    void RecyclePlayer(FPlayerHandle &&player);

    /// Remove The Not Login Player Agent
    void RemoveAgent(const std::string &key);

    /// Handle On Player Login
    void OnPlayerLogin(const std::string &key, int64_t pid);

protected:
    void Initial() override;
    void Start() override;
    void Stop() override;

private:
    awaitable<void> WaitForClient(uint16_t port);
    awaitable<void> CollectCachedPlayer();
};
