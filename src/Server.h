#pragma once

#include "Module.h"
#include "PlayerBase.h"
#include "base/MultiIOContextPool.h"
#include "base/SingleIOContextPool.h"
#include "base/IdentAllocator.h"
#include "factory/CodecFactory.h"
#include "factory/PlayerFactory.h"
#include "factory/ServiceFactory.h"

#include <typeindex>
#include <shared_mutex>
#include <yaml-cpp/yaml.h>
#include <absl/container/flat_hash_map.h>
#include <vector>


enum class EServerState {
    CREATED,
    INITIALIZED,
    RUNNING,
    TERMINATED,
};


class UServiceAgent;
class IDataAsset_Interface;

class BASE_API UServer final {

    struct BASE_API FCachedNode {
        FPlayerHandle player;
        ASteadyTimePoint timepoint;
    };

    using AContextMap = absl::flat_hash_map<int64_t, shared_ptr<UServiceAgent>>;

public:
    UServer();
    ~UServer();

    DISABLE_COPY_MOVE(UServer)

    [[nodiscard]] EServerState GetState() const;

    void Initial();
    void Start();
    void Shutdown();

    [[nodiscard]] asio::io_context& GetIOContext();
    [[nodiscard]] asio::io_context& GetWorkerContext();

    template<class T>
    requires std::derived_from<T, IModuleBase>
    T *CreateModule() {
        if (mModuleMap.contains(typeid(T)))
            throw std::logic_error("Module Already Exists");

        auto module = std::make_unique<T>();
        auto *pModule = module.get();

        module->SetUpModule(this);

        mModuleMap.emplace(typeid(T), std::move(module));
        mModuleOrder.emplace_back(pModule);

        return pModule;
    }

    template<class T>
    requires std::derived_from<T, IModuleBase>
    T *GetModule() const {
        const auto it = mModuleMap.find(typeid(T));
        return it == mModuleMap.end() ? nullptr : dynamic_cast<T *>(it->second.get());
    }

    template<class T, class... Args>
    requires std::derived_from<T, ICodecFactory_Interface>
    void SetCodecFactory(Args && ... args) {
        if (mState != EServerState::CREATED)
            throw std::logic_error("Only can set CodecFactory in CREATED state");

        mCodecFactory = make_unique<T>(std::forward<Args>(args)...);
    }

    template<class T, class... Args>
    requires std::derived_from<T, IPlayerFactory_Interface>
    void SetPlayerFactory(Args && ... args) {
        if (mState != EServerState::CREATED)
            throw std::logic_error("Only can set PlayerFactory in CREATED state");

        mPlayerFactory = make_unique<T>(std::forward<Args>(args)...);
    }

    template<class T, class... Args>
    requires std::derived_from<T, IServiceFactory_Interface>
    void SetServiceFactory(Args && ... args) {
        if (mState != EServerState::CREATED)
            throw std::logic_error("Only can set ServiceFactory in CREATED state");

        mServiceFactory = make_unique<T>(std::forward<Args>(args)...);
    }

    [[nodiscard]] const YAML::Node &GetServerConfig() const;

    unique_ptr<IPackageCodec_Interface> CreateUniquePackageCodec(ATcpSocket &&socket) const;
    unique_ptr<IRecyclerBase> CreateUniquePackagePool(asio::io_context &ctx) const;

    FPlayerHandle CreatePlayer() const;
    unique_ptr<IAgentHandler> CreateAgentHandler() const;

    [[nodiscard]] ICodecFactory_Interface *GetCodecFactory() const;
    [[nodiscard]] IPlayerFactory_Interface *GetPlayerFactory() const;
    [[nodiscard]] IServiceFactory_Interface *GetServiceFactory() const;

    [[nodiscard]] shared_ptr<UPlayerAgent> FindPlayer(int64_t pid) const;
    [[nodiscard]] shared_ptr<UPlayerAgent> FindAgent(const std::string &key) const;

    [[nodiscard]] std::vector<shared_ptr<UPlayerAgent>> GetPlayerList(const std::vector<int64_t> &list) const;

    void RemovePlayer(int64_t pid);
    void RecyclePlayer(FPlayerHandle &&player);

    void RemoveAgent(const std::string &key);

    void OnPlayerLogin(const std::string &key, int64_t pid);

    [[nodiscard]] shared_ptr<UServiceAgent> FindService(int64_t sid) const;
    [[nodiscard]] shared_ptr<UServiceAgent> FindService(const std::string &name) const;

    void BootService(const std::string &path, const IDataAsset_Interface *pData);

    void ShutdownService(int64_t sid);
    void ShutdownService(const std::string &name);

private:
    awaitable<void> WaitForClient(uint16_t port);
    awaitable<void> CollectCachedPlayer();

private:
#pragma region IO Management
    asio::io_context        mIOContext;
    ATcpAcceptor            mAcceptor;
    UMultiIOContextPool     mIOContextPool;
#pragma endregion IO Manage

#pragma region Agent Management
    absl::flat_hash_map<std::string, shared_ptr<UPlayerAgent>> mAgentMap;
    mutable std::shared_mutex mAgentMutex;

    absl::flat_hash_map<int64_t, shared_ptr<UPlayerAgent>> mPlayerMap;
    mutable std::shared_mutex mPlayerMutex;

    absl::flat_hash_map<int64_t, FCachedNode> mCachedMap;
    mutable std::shared_mutex mCacheMutex;

    ASteadyTimer mCacheTimer;
#pragma endregion

#pragma region Service Management
    USingleIOContextPool mWorkerPool;
    AContextMap mServiceMap;
    mutable std::shared_mutex mServiceMutex;

    absl::flat_hash_map<std::string, int64_t> mServiceNameMap;
    mutable std::shared_mutex mServiceNameMutex;

    TIdentAllocator<int64_t, true> mAllocator;
#pragma endregion

#pragma region Module Define
    absl::flat_hash_map<std::type_index, std::unique_ptr<IModuleBase>> mModuleMap;
    std::vector<IModuleBase *> mModuleOrder;
#pragma endregion

    unique_ptr<ICodecFactory_Interface> mCodecFactory;
    unique_ptr<IPlayerFactory_Interface> mPlayerFactory;
    unique_ptr<IServiceFactory_Interface> mServiceFactory;

    EServerState mState;
};

