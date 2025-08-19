#include "Server.h"
#include "base/PackageCodec.h"
#include "base/Recycler.h"

#include <spdlog/spdlog.h>
#include <format>

// UServer::UServer()
//     : mWorkGuard(asio::make_work_guard(mIOContext)),
//       bInitialized(false),
//       bRunning(false),
//       bShutdown(false) {
// }
//
// UServer::~UServer() {
//     for (auto &th: mWorkerList) {
//         if (th.joinable()) {
//             th.join();
//         }
//     }
// }
//
// asio::io_context &UServer::GetIOContext() {
//     return mIOContext;
// }
//
// IModuleBase *UServer::GetModule(const std::string &name) const {
//     const auto iter = mNameToModule.find(name);
//     if (iter == mNameToModule.end()) {
//         return nullptr;
//     }
//
//     const auto module_iter = mModuleMap.find(iter->second);
//     return module_iter != mModuleMap.end() ? module_iter->second.get() : nullptr;
// }
//
// void UServer::Initial() {
//     if (bInitialized)
//         return;
//
//     SPDLOG_INFO("Loading Service From {}", CORE_SERVICE_DIRECTORY);
//
//     if (!std::filesystem::exists(PLAYER_AGENT_DIRECTORY)) {
//         try {
//             std::filesystem::create_directory(PLAYER_AGENT_DIRECTORY);
//         } catch (const std::exception &e) {
//             SPDLOG_CRITICAL("{:<20} - Failed To Create Player Agent Directory {}", __FUNCTION__, e.what());
//             Shutdown();
//             exit(-1);
//         }
//     }
//
//     if (!std::filesystem::exists(CORE_SERVICE_DIRECTORY)) {
//         try {
//             std::filesystem::create_directory(CORE_SERVICE_DIRECTORY);
//         } catch (const std::exception &e) {
//             SPDLOG_CRITICAL("{:<20} - Failed To Create Service Directory {}", __FUNCTION__, e.what());
//             Shutdown();
//             exit(-2);
//         }
//     }
//
//     if (!std::filesystem::exists(EXTEND_SERVICE_DIRECTORY)) {
//         try {
//             std::filesystem::create_directory(EXTEND_SERVICE_DIRECTORY);
//         } catch (const std::exception &e) {
//             SPDLOG_CRITICAL("{:<20} - Failed To Create Extend Directory {}", __FUNCTION__, e.what());
//             Shutdown();
//             exit(-3);
//         }
//     }
//
//     SPDLOG_INFO("Initializing Modules...");
//     for (const auto &type : mModuleOrder) {
//         if (auto *module = mModuleMap[type].get()) {
//             module->Initial();
//             SPDLOG_INFO("{} Initialized.", module->GetModuleName());
//         }
//     }
//     SPDLOG_INFO("Modules Initialization Completed!");
//
//     const auto *config = GetModule<UConfig>();
//     if (config == nullptr) {
//         SPDLOG_CRITICAL("Fail To Found Config Module!");
//         Shutdown();
//         exit(-4);
//     }
//
//     //const int count = config->GetServerConfig()["server"]["worker"].as<int>();
//     const int count = 4;
//     for (int idx = 1; idx < count; ++idx) {
//         mWorkerList.emplace_back([this, idx] {
//             const int64_t tid = utils::ThreadIDToInt(std::this_thread::get_id());
//
//             SPDLOG_INFO("Worker[{}] Is Running In Thread: {}", idx, tid);
//             mIOContext.run();
//         });
//     }
//     SPDLOG_INFO("Server Run With {} Worker(s)", count);
//     SPDLOG_INFO("Server Initialization Completed!");
//
//     bInitialized = true;
// }

UServer::UServer()
    : mState(EServerState::CREATED) {
}

UServer::~UServer() {
}

EServerState UServer::GetState() const {
    return mState;
}

void UServer::Initial() {
    if (mState != EServerState::CREATED)
        throw std::logic_error(std::format("{} - Server Not In CREATED State", __FUNCTION__));

    for (const auto &pModule : mModuleOrder) {
        pModule->Initial();
    }

    mState = EServerState::INITIALIZED;
}

void UServer::Start() {
    if (mState != EServerState::INITIALIZED)
        throw std::logic_error(std::format("{} - Server Not In INITIALIZED State", __FUNCTION__));

    for (const auto &pModule : mModuleOrder) {
        pModule->Start();
    }

    mState = EServerState::RUNNING;
}

void UServer::Shutdown() {
    if (mState == EServerState::TERMINATED)
        return;

    mState = EServerState::TERMINATED;

    for (auto iter = mModuleOrder.rbegin(); iter != mModuleOrder.rend(); ++iter) {
        (*iter)->Stop();
    }
}

unique_ptr<IPackageCodec_Interface> UServer::CreateUniquePackageCodec(ATcpSocket &&socket) const {
    if (mCodecFactory == nullptr)
        return nullptr;

    return mCodecFactory->CreateUniquePackageCodec(std::move(socket));
}

unique_ptr<IRecyclerBase> UServer::CreateUniquePackagePool() const {
    if (mCodecFactory == nullptr)
        return nullptr;

    return mCodecFactory->CreateUniquePackagePool();
}
