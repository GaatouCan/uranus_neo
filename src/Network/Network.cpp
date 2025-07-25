#include "Network.h"
#include "Base/Package.h"

#include <string>
#include <spdlog/spdlog.h>


static const std::string key = "1d0fa7879b018b71a00109e7e29eb0c5037617e9eddc3d46e95294fc1fa3ad50";

UNetwork::UNetwork()
    : mAcceptor(mIOContext),
      mPackagePool(nullptr) {
}

UNetwork::~UNetwork() {
    Stop();

    if (mThread.joinable()) {
        mThread.join();
    }
}

void UNetwork::Initial() {
    if (mState != EModuleState::CREATED)
        return;

    mPackagePool = make_shared<TRecycler<FPackage>>();
    mPackagePool->Initial();

    mThread = std::thread([this] {
       // SPDLOG_INFO("Network Work In Thread[{}]", utils::ThreadIDToInt(std::this_thread::get_id()));

       asio::signal_set signals(mIOContext, SIGINT, SIGTERM);
       signals.async_wait([this](auto, auto) {
           Stop();
       });
       mIOContext.run();
   });

    mState = EModuleState::INITIALIZED;
}

void UNetwork::Start() {
    if (mState != EModuleState::INITIALIZED)
        return;

    uint16_t port = 8080;
    // if (const auto *config = GetServer()->GetModule<UConfig>(); config != nullptr) {
    //     port = config->GetServerConfig()["server"]["port"].as<uint16_t>();
    // }

    co_spawn(mIOContext, WaitForClient(port), detached);
    mState = EModuleState::RUNNING;
}

void UNetwork::Stop() {
    if (mState == EModuleState::STOPPED)
        return;

    mState = EModuleState::STOPPED;

    if (!mIOContext.stopped()) {
        mIOContext.stop();
    }

    mConnectionMap.clear();
}

io_context &UNetwork::GetIOContext() {
    return mIOContext;
}

awaitable<void> UNetwork::WaitForClient(uint16_t port) {
    co_return;
}

std::shared_ptr<FPackage> UNetwork::BuildPackage() {
    return std::dynamic_pointer_cast<FPackage>(mPackagePool->Acquire());
}
