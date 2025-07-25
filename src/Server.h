#pragma once

#include "Module.h"


#include <typeindex>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <atomic>
#include <asio.hpp>

class BASE_API UServer final {

#pragma region Module Define
    std::unordered_map<std::type_index, std::unique_ptr<IModuleBase>> mModuleMap;
    std::unordered_map<std::string, std::type_index> mNameToModule;
    std::vector<std::type_index> mModuleOrder;
#pragma endregion

#pragma region Worker
    asio::io_context mIOContext;
    asio::executor_work_guard<asio::io_context::executor_type> mWorkGuard;
    std::vector<std::thread> mWorkerList;
#pragma endregion

    std::atomic_bool bInitialized;
    std::atomic_bool bRunning;
    std::atomic_bool bShutdown;

public:
    UServer();
    ~UServer();

    DISABLE_COPY_MOVE(UServer)

    [[nodiscard]] asio::io_context &GetIOContext();

    template<CModuleType ModuleType, typename... Args>
    ModuleType *CreateModule(Args &&... args);

    template<CModuleType ModuleType>
    ModuleType *GetModule() const;

    IModuleBase *GetModule(const std::string &name) const;

    void Initial();
    void Run();
    void Shutdown();

    [[nodiscard]] bool IsInitialized() const;
    [[nodiscard]] bool IsRunning() const;
    [[nodiscard]] bool IsShutdown() const;
};

template<CModuleType ModuleType, typename ... Args>
inline ModuleType *UServer::CreateModule(Args &&...args) {
    if (bInitialized || bRunning || bShutdown) {
        return nullptr;
    }

    if (mModuleMap.contains(typeid(ModuleType))) {
        return dynamic_cast<ModuleType *>(mModuleMap[typeid(ModuleType)].get());
    }

    auto *pModule = new ModuleType(std::forward<Args>(args)...);
    auto ptr = std::unique_ptr<IModuleBase>(pModule);

    ptr->SetUpModule(this);

    mNameToModule.emplace(ptr->GetModuleName(), typeid(ModuleType));
    mModuleOrder.emplace_back(typeid(ModuleType));

    mModuleMap.emplace(typeid(ModuleType), std::move(ptr));

    return pModule;
}

template<CModuleType ModuleType>
inline ModuleType *UServer::GetModule() const {
    const auto iter = mModuleMap.find(typeid(ModuleType));
    return iter != mModuleMap.end() ? dynamic_cast<ModuleType *>(iter->second.get()) : nullptr;
}
