#include "DataAccess.h"
#include "DatabaseTask.h"

#include <mongocxx/uri.hpp>
#include <spdlog/spdlog.h>

#include "Server.h"

UDataAccess::UDataAccess()
    : mNextIndex(0) {
}

void UDataAccess::Initial() {
    if (mState != EModuleState::CREATED)
        return;

    mPool = std::make_unique<mongocxx::pool>(
        mongocxx::uri("mongodb://username:12345678@localhost:27017/demo?maxPoolSize=10"));

    mWorkerList = std::vector<FWorkerNode>(2);
    for (auto &worker: mWorkerList) {
        worker.thread = std::thread([this, &worker] {
            auto client = mPool->acquire();
            auto db = client["demo"];

            while (worker.deque.IsRunning() && mState == EModuleState::RUNNING) {
                worker.deque.Wait();

                if (!worker.deque.IsRunning() || mState != EModuleState::RUNNING)
                    break;

                try {
                    const auto node = std::move(worker.deque.PopFront());
                    node->Execute(db);
                } catch (const std::exception &e) {
                    SPDLOG_ERROR("UDataAccess::RunInThread - Exception: {}", e.what());
                }
            }

            worker.deque.Clear();
        });
    }

    {
        asio::co_spawn(GetServer()->GetIOContext(), [this]() -> asio::awaitable<void> {
            auto builder = bsoncxx::builder::basic::document();
            builder.append(bsoncxx::builder::basic::kvp("player_id", 1001));
            auto doc = builder.extract();
            auto res = co_await AsyncSelect("player", doc, asio::use_awaitable);
        }, asio::detached);
    }

    mState = EModuleState::INITIALIZED;
}

void UDataAccess::Stop() {
    if (mState == EModuleState::STOPPED)
        return;

    mState = EModuleState::STOPPED;

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
