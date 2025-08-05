#pragma once

#include "Database/DBAdapterBase.h"
#include "ConcurrentDeque.h"

#include <mongocxx/instance.hpp>
#include <mongocxx/pool.hpp>


class IDBTaskBase;

class BASE_API UMongoAdapter final : public IDBAdapterBase {

public:
    UMongoAdapter();
    ~UMongoAdapter() override;

    void Initial(const IDataAsset_Interface *data) override;

    IDBContextBase *AcquireContext() override;

private:
    mongocxx::instance mInstance;
    std::unique_ptr<mongocxx::pool> mPool;
    std::string mDatabaseName;
};
