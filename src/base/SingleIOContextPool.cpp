#include "SingleIOContextPool.h"

#include <asio/signal_set.hpp>

USingleIOContextPool::USingleIOContextPool()
    : mGuard(asio::make_work_guard(mIOContext)) {
}

USingleIOContextPool::~USingleIOContextPool() {
    for (auto &val: mThreadList) {
        if (val.joinable()) {
            val.join();
        }
    }
}

void USingleIOContextPool::Start(const size_t capacity) {
    mThreadList = std::vector<std::thread>(capacity);
    for (auto &val: mThreadList) {
        val = std::thread([this] {
            mIOContext.run();
        });
    }

    asio::signal_set signals(mIOContext, SIGINT, SIGTERM);
    signals.async_wait([this](auto, auto) {
        mIOContext.stop();
    });
}

asio::io_context &USingleIOContextPool::GetIOContext() {
    return mIOContext;
}
