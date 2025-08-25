#pragma once

#include "Common.h"

#include <asio/io_context.hpp>
#include <vector>
#include <atomic>
#include <thread>


class BASE_API UMultiIOContextPool final {

    struct FPoolNode {
        std::thread thread;
        asio::io_context context;
        asio::executor_work_guard<asio::io_context::executor_type> guard;

        FPoolNode();
    };

public:
    UMultiIOContextPool();
    ~UMultiIOContextPool();

    DISABLE_COPY_MOVE(UMultiIOContextPool)

    void Start(size_t count);
    void Stop();

    asio::io_context& GetIOContext();

private:
    std::vector<FPoolNode> mNodeList;
    std::atomic_size_t mNextIndex;
};
