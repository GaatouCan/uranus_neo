#include "DataAccess.h"

#include <mongocxx/uri.hpp>
#include <spdlog/spdlog.h>

UDataAccess::UDataAccess() {
}

void UDataAccess::Initial() {
    if (mState != EModuleState::CREATED)
        return;

    mPool = std::make_unique<mongocxx::pool>(mongocxx::uri("mongodb://username:12345678@localhost:27017/demo?maxPoolSize=10"));

    mWorkerList = std::vector<FWorkerNode>(2);
    for (auto &worker : mWorkerList) {
        worker.thread = std::thread([this, &worker] {
            auto client = mPool->acquire();
            auto db = client["demo"];

            while (worker.deque.IsRunning() && mState == EModuleState::RUNNING) {
                worker.deque.Wait();

                if (!worker.deque.IsRunning() || mState != EModuleState::RUNNING)
                    break;

                try {
                    const auto node = std::move(worker.deque.PopFront());
                    // TODO: Real Handle Database Task
                } catch (const std::exception &e) {
                    SPDLOG_ERROR("UDataAccess::RunInThread - Exception: {}", e.what());
                }
            }

            worker.deque.Clear();
        });
    }

    mState = EModuleState::INITIALIZED;
}

void UDataAccess::Stop() {
    if (mState == EModuleState::STOPPED)
        return;

    mState = EModuleState::STOPPED;

    for (auto &[thread, deque] : mWorkerList) {
        deque.Quit();
    }
}

UDataAccess::~UDataAccess() {
    for (auto &[thread, deque] : mWorkerList) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}
