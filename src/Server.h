#pragma once

#include "Module.h"
#include "base/MultiIOContextPool.h"
#include "base/CodecFactory.h"
#include "base/PlayerFactory.h"

#include <typeindex>
#include <shared_mutex>
#include <absl/container/flat_hash_map.h>


enum class EServerState {
    CREATED,
    INITIALIZED,
    RUNNING,
    TERMINATED,
};

class BASE_API UServer final {

public:
    UServer();
    ~UServer();

    DISABLE_COPY_MOVE(UServer)

    [[nodiscard]] EServerState GetState() const;

    void Initial();
    void Start();
    void Shutdown();

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

    unique_ptr<IPackageCodec_Interface> CreateUniquePackageCodec(ATcpSocket &&socket) const;
    unique_ptr<IRecyclerBase> CreateUniquePackagePool() const;

    unique_ptr<IPlayerBase> CreatePlayer() const;
    unique_ptr<IAgentHandler> CreateAgentHandler() const;

    [[nodiscard]] shared_ptr<UAgent> FindAgent(int64_t pid) const;
    [[nodiscard]] shared_ptr<UAgent> FindAgent(const std::string &key) const;

    void RemoveAgent(std::unique_ptr<IPlayerBase> &&player);

    void OnPlayerLogin(const std::string &key, int64_t pid);

private:
    awaitable<void> WaitForClient(uint16_t port);

private:
#pragma region IO Management
    io_context              mIOContext;
    ATcpAcceptor            mAcceptor;
    UMultiIOContextPool     mIOContextPool;
#pragma endregion IO Manage

#pragma region Agent Management
    absl::flat_hash_map<std::string, shared_ptr<UAgent>> mAgentMap;
    absl::flat_hash_map<int64_t, shared_ptr<UAgent>> mPlayerMap;
    mutable std::shared_mutex mAgentMutex;
#pragma endregion

#pragma region Module Define
    absl::flat_hash_map<std::type_index, std::unique_ptr<IModuleBase>> mModuleMap;
    std::vector<IModuleBase *> mModuleOrder;
#pragma endregion

    unique_ptr<ICodecFactory_Interface> mCodecFactory;
    unique_ptr<IPlayerFactory_Interface> mPlayerFactory;

    EServerState mState;
};

