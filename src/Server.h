#pragma once

#include "Module.h"
#include "factory/CodecFactory.h"

#include <typeindex>
#include <yaml-cpp/yaml.h>
#include <absl/container/flat_hash_map.h>


enum class EServerState {
    CREATED,
    INITIALIZED,
    RUNNING,
    TERMINATED,
};

class BASE_API UServer final {
    /** The Basic IOContext Of The Whole Server **/
    asio::io_context mIOContext;

    /** The Module Map With The Key Of typeinfo **/
    absl::flat_hash_map<std::type_index, std::unique_ptr<IModuleBase> > mModuleMap;

    /** Record The Order Of Declare The Modules **/
    std::vector<IModuleBase *> mModuleOrder;

    /** The Factory To Create The Package Code And Package Pool **/
    unique_ptr<ICodecFactory_Interface> mCodecFactory;

    /** Current Server State **/
    EServerState mState;

public:
    UServer();
    ~UServer();

    DISABLE_COPY_MOVE(UServer)

    /// Return The Current Server State
    [[nodiscard]] EServerState GetState() const;

    /// Initial All The Modules
    void Initial();

    /// Start All The Modules And Run The Main IOContext
    void Start();

    /// Stop All The Modules And The Main IOContext
    void Shutdown();

    /// Get The Reference Of The Main IOContext
    [[nodiscard]] asio::io_context &GetIOContext();

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
    void SetCodecFactory(Args &&... args) {
        if (mState != EServerState::CREATED)
            throw std::logic_error("Only can set CodecFactory in CREATED state");

        mCodecFactory = make_unique<T>(std::forward<Args>(args)...);
    }

    /// Return The Server Configuration
    [[nodiscard]] const YAML::Node &GetServerConfig() const;

    /// Create A New PackageCodec With A Tcp Socket Use Inner Factory
    unique_ptr<IPackageCodec_Interface> CreateUniquePackageCodec(ATcpSocket &&socket) const;

    /// Create A New PackagePool And Bind To The Specified IOContext, Use Inner Factory
    unique_ptr<IRecyclerBase> CreateUniquePackagePool(asio::io_context &ctx) const;

    /// Get The Raw Pointer Of Inner Codec Factory
    [[nodiscard]] ICodecFactory_Interface *GetCodecFactory() const;
};
