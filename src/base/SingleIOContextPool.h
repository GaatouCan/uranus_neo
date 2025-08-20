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


private:
    asio::io_context mIOContext;
    asio::executor_work_guard<asio::io_context::executor_type> mGuard;

};
