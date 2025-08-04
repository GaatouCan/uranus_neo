#pragma once

#include "Database/DBAdapter.h"
#include "ConcurrentDeque.h"

#include <thread>
#include <vector>
#include <mongocxx/instance.hpp>
#include <mongocxx/pool.hpp>


class IDBTaskBase;

class BASE_API UMongoAdapter final : public IDBAdapter_Interface {

public:
    UMongoAdapter();
    ~UMongoAdapter() override;

    void Initial(const IDataAsset_Interface *data) override;

private:
    mongocxx::instance mInstance;
    std::unique_ptr<mongocxx::pool> mPool;

    struct FWorkerNode {
        std::thread thread;
        TConcurrentDeque<std::unique_ptr<IDBTaskBase>, true> deque;
    };

    std::vector<FWorkerNode> mWorkerList;
    std::atomic_size_t mNextIndex;
};
