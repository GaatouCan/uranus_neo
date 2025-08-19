#include "MultiIOContextPool.h"

#include <asio/signal_set.hpp>


UMultiIOContextPool::FPoolNode::FPoolNode()
    : guard(asio::make_work_guard(context)) {
}

UMultiIOContextPool::UMultiIOContextPool()
    : mNextIndex(0) {
}

UMultiIOContextPool::~UMultiIOContextPool() {
    Stop();

    for (auto &node: mNodeList) {
        if (node.thread.joinable()) {
            node.thread.join();
        }
    }
}

void UMultiIOContextPool::Start(const size_t count) {
    mNodeList = std::vector<FPoolNode>(count);
    for (auto &node: mNodeList) {
        node.thread = std::thread([&node] {
            asio::signal_set signals(node.context, SIGINT, SIGTERM);
            signals.async_wait([&node](auto, auto) {
                node.guard.reset();
                node.context.stop();
            });

            node.context.run();
        });
    }
}

void UMultiIOContextPool::Stop() {
    for (auto &node: mNodeList) {
        node.guard.reset();
        node.context.stop();
    }
}

asio::io_context &UMultiIOContextPool::GetIOContext() {
    if (mNodeList.empty())
        throw std::runtime_error("No IOContext");

    auto &node = mNodeList[mNextIndex++];
    mNextIndex = mNextIndex % mNodeList.size();

    return node.context;
}
