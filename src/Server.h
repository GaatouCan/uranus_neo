#pragma once

#include "base/Types.h"
#include "base/MultiIOContextPool.h"


class UNetwork;

class BASE_API UServer final {


public:
    UServer();
    ~UServer();

    DISABLE_COPY_MOVE(UServer)

    void Initial();
    void Start();
    void Shutdown();

private:
    awaitable<void> WaitForClient();

private:
    asio::io_context mIOContext;
    ATcpAcceptor mAcceptor;
    UMultiIOContextPool mPool;

    UNetwork *mNetwork;
};

