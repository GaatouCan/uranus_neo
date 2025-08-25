#pragma once

#include "Common.h"

#include <asio/io_context.hpp>
#include <vector>
#include <thread>

class BASE_API USingleIOContextPool final {

public:
    USingleIOContextPool();
    ~USingleIOContextPool();

    DISABLE_COPY_MOVE(USingleIOContextPool);

    void Start(size_t capacity = 4);
    void Stop();

    [[nodiscard]] asio::io_context& GetIOContext();

private:
    asio::io_context mIOContext;
    asio::executor_work_guard<asio::io_context::executor_type> mGuard;
    std::vector<std::thread> mThreadList;
};


