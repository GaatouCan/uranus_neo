#pragma once

#include "Module.h"
#include "ConcurrentDeque.h"

#include <thread>
#include <vector>
#include <mongocxx/instance.hpp>
#include <mongocxx/pool.hpp>


class BASE_API UDataAccess final : public IModuleBase {

    DECLARE_MODULE(UDataAccess)

protected:
    UDataAccess();

    void Initial() override;
    void Stop() override;

public:
    ~UDataAccess() override;

    [[nodiscard]] constexpr const char *GetModuleName() const override {
        return "Data Access";
    }

private:
    mongocxx::instance mInstance;
    std::unique_ptr<mongocxx::pool> mPool;

    struct FWorkerNode {
        std::thread thread;
        TConcurrentDeque<int, true> deque;
    };

    std::vector<FWorkerNode> mWorkerList;
};

