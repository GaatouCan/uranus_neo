#pragma once

#include "Database/DBAdapterBase.h"

#include "DBTask_InsertOne.h"

#include <mongocxx/instance.hpp>
#include <mongocxx/pool.hpp>


class IDBTaskBase;

class BASE_API UMongoAdapter final : public IDBAdapterBase {

public:
    UMongoAdapter();
    ~UMongoAdapter() override;

    void Initial(const IDataAsset_Interface *data) override;

    IDBContext_Interface *AcquireContext() override;

    template<class Callback = decltype(NoOperateCallback)>
    void PushInsertOne(const std::string &col, const bsoncxx::document::value &doc, const mongocxx::options::insert &opt, Callback &&cb = NoOperateCallback) {
        auto task = std::make_unique<mongo::TDBTask_InsertOne<Callback>>(mDatabaseName, col, doc, opt, std::forward<Callback>(cb));
        this->PushTask(std::move(task));
    }

private:
    mongocxx::instance mInstance;
    std::unique_ptr<mongocxx::pool> mPool;
    std::string mDatabaseName;
};
