#include "DataAccess.h"

#include <mongocxx/uri.hpp>
#include <spdlog/spdlog.h>

UDataAccess::UDataAccess() {
}

void UDataAccess::Initial() {
    if (mState != EModuleState::CREATED)
        return;

    mClient = mongocxx::client(mongocxx::uri("mongodb://username:12345678@localhost:27017/demo?maxPoolSize=10"));

    mThread = std::thread([this] {
        while (mDeque.IsRunning() && mState == EModuleState::RUNNING) {
            mDeque.Wait();

            if (!mDeque.IsRunning() && mState != EModuleState::RUNNING)
                break;

            try {
                const auto node = std::move(mDeque.PopFront());
                // TODO: Real Handle Database Task

            } catch (const std::exception &e) {
                SPDLOG_ERROR("UDataAccess::RunInThread - Exception: {}", e.what());
            }
        }

        mDeque.Clear();
    });

    mState = EModuleState::INITIALIZED;
}

void UDataAccess::Stop() {
    if (mState == EModuleState::STOPPED)
        return;

    mState = EModuleState::STOPPED;

    mDeque.Quit();
}

UDataAccess::~UDataAccess() {
    if (mThread.joinable()) {
        mThread.join();
    }
}
