#include "SingleIOContextPool.h"

USingleIOContextPool::USingleIOContextPool()
    : mGuard(asio::make_work_guard(mIOContext)){
}

USingleIOContextPool::~USingleIOContextPool() {
}

