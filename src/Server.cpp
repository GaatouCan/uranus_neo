#include "Server.h"

#include <spdlog/spdlog.h>

#include "network/Network.h"


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
    : mAcceptor(mIOContext) {

    mNetwork = new UNetwork();
    mNetwork->SetUpModule(this);
}

UServer::~UServer() {

    delete mNetwork;
}

void UServer::Initial() {
}

void UServer::Start() {

    asio::signal_set signals(mIOContext, SIGINT, SIGTERM);
    signals.async_wait([this](auto, auto) {
        Shutdown();
    });

    co_spawn(mIOContext, WaitForClient(), detached);

    mIOContext.run();
}

void UServer::Shutdown() {
    if (!mIOContext.stopped())
        mIOContext.stop();
}

awaitable<void> UServer::WaitForClient() {
    constexpr int port = 8080;

    try {
        mAcceptor.open(asio::ip::tcp::v4());
        mAcceptor.bind({asio::ip::tcp::v4(), port});
        mAcceptor.listen(port);

        while (!mIOContext.stopped()) {
            auto [ec, socket] = co_await mAcceptor.async_accept();
            if (ec) {
                SPDLOG_ERROR("{:<20} - {}", __FUNCTION__, ec.message());
                continue;
            }

            if (!socket.is_open())
                continue;

            auto addr = socket.remote_endpoint().address();
            // TODO: Check Address

            mNetwork->OnAccept(std::move(socket));
        }
    } catch (const std::exception &e) {
    }
}

// void UServer::Run() {
//     if (bRunning)
//         return;
//
//     for (const auto &type : mModuleOrder) {
//         if (auto *module = mModuleMap[type].get()) {
//             module->Start();
//             SPDLOG_INFO("{} Started.", module->GetModuleName());
//         }
//     }
//
//     asio::signal_set signals(mIOContext, SIGINT, SIGTERM);
//     signals.async_wait([this](auto, auto) {
//         Shutdown();
//     });
//
//     mIOContext.run();
// }
//
// void UServer::Shutdown() {
//     if (bShutdown)
//         return;
//
//     bShutdown = true;
//     SPDLOG_INFO("Shutting Down...");
//
//     for (auto iter = mModuleOrder.rbegin(); iter != mModuleOrder.rend(); ++iter) {
//         if (auto *module = mModuleMap[*iter].get()) {
//             module->Stop();
//             SPDLOG_INFO("{} Stopped.", module->GetModuleName());
//         }
//     }
//
//     mWorkGuard.reset();
//     mIOContext.stop();
//
//     SPDLOG_INFO("Server Shutdown Completed!");
// }
