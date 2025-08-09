#include "DataAccess.h"
#include "DBTaskBase.h"
#include "DBContext.h"
#include "Server.h"
#include "Config/Config.h"

#include <spdlog/spdlog.h>

UDataAccess::UDataAccess()
    : mNextIndex(0) {
}

void UDataAccess::Initial() {
    if (mState != EModuleState::CREATED)
        return;

    assert(mAdapter != nullptr);
    assert(mInitConfig != nullptr);

    const auto *module = GetServer()->GetModule<UConfig>();
    assert(module != nullptr);

    const auto *startUp = std::invoke(mInitConfig, module->GetServerConfig());
    mAdapter->Initial(startUp);

    mWorkerList = std::vector<FWorkerNode>(2);
    for (auto &worker: mWorkerList) {
        worker.thread = std::thread([this, &worker] {
            auto *ctx = mAdapter->AcquireContext();

            while (worker.deque.IsRunning() && mState == EModuleState::RUNNING) {
                worker.deque.Wait();

                if (!worker.deque.IsRunning() || mState != EModuleState::RUNNING)
                    break;

                try {
                    const auto node = std::move(worker.deque.PopFront());
                    node->Execute(ctx);
                } catch (const std::exception &e) {
                    SPDLOG_ERROR("UDataAccess::RunInThread - Exception: {}", e.what());
                }
            }

            worker.deque.Clear();
            delete ctx;
        });
    }

    delete startUp;
    mState = EModuleState::INITIALIZED;
}

void UDataAccess::Stop() {
    if (mState == EModuleState::STOPPED)
        return;

    mState = EModuleState::STOPPED;

    mAdapter->Stop();

    for (auto &[thread, deque]: mWorkerList) {
        deque.Quit();
    }
}

UDataAccess::~UDataAccess() {
    for (auto &[thread, deque]: mWorkerList) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

void UDataAccess::SetStartUpConfig(const std::function<IDataAsset_Interface *(const YAML::Node &)> &func) {
    mInitConfig = func;
}

IDBAdapterBase *UDataAccess::GetAdapter() const {
    if (mAdapter == nullptr)
        return nullptr;
    return mAdapter.get();
}

void UDataAccess::PushTask(std::unique_ptr<IDBTaskBase> &&task) {
    if (mState != EModuleState::RUNNING)
        return;

    if (mWorkerList.empty())
        return;
    auto &[thread, deque] = mWorkerList[mNextIndex++];
    mNextIndex = mNextIndex % mWorkerList.size();

    deque.PushBack(std::move(task));
}
