#pragma once

#include "Database/DBAdapterBase.h"

#include "DBTask_InsertOne.h"
#include "DBTask_InsertMany.h"
#include "DBTask_UpdateOne.h"
#include "DBTask_UpdateMany.h"
#include "DBTask_Replace.h"
#include "DBTask_DeleteOne.h"
#include "DBTask_DeleteMany.h"
#include "DBTask_FindOne.h"
#include "MongoTaskDef.h"
#include "DBTask_Transcation.h"

#include <atomic>
#include <mongocxx/instance.hpp>
#include <mongocxx/pool.hpp>
#include <asio.hpp>


#define MONGO_OPERATE_DEFINE(OP, PARAMS, ARGS, RET, FAILURE)                                                                                                \
template<class Callback = decltype(NoOperateCallback)>                                                                                                      \
void Push##OP(const std::string &col, PARAMS, Callback &&cb = NoOperateCallback) {                                                                          \
    auto task = std::make_unique<mongo::TDBTask_##OP<Callback>>(mDatabaseName, col, std::forward<Callback>(cb), ARGS);                                      \
    this->PushTask(std::move(task));                                                                                                                        \
}                                                                                                                                                           \
template<asio::completion_token_for<void(RET)> CompletionToken>                                                                                             \
auto Async##OP(const std::string &col, PARAMS, CompletionToken &&token) {                                                                                   \
    auto init = [this](asio::completion_handler_for<void(RET)> auto handle, const std::string &col, PARAMS) {                                               \
        auto work = asio::make_work_guard(handle);                                                                                                          \
        if (bQuit) {                                                                                                                                        \
            auto alloc = asio::get_associated_allocator(handle, asio::recycling_allocator<void>());                                                         \
            asio::dispatch(work.get_executor(), asio::bind_allocator(alloc, [handle = std::move(handle)]() mutable {                                        \
                std::move(handle)(FAILURE);                                                                                                                 \
            }));                                                                                                                                            \
            return;                                                                                                                                         \
        }                                                                                                                                                   \
        this->Push##OP(col, ARGS, [handle = std::move(handle), work = std::move(work)](RET result) mutable {                                                \
            auto alloc = asio::get_associated_allocator(handle, asio::recycling_allocator<void>());                                                         \
            asio::dispatch(work.get_executor(), asio::bind_allocator(alloc, [handle = std::move(handle), result = std::forward<RET>(result)]() mutable {    \
                std::move(handle)(std::move(result));                                                                                                       \
            }));                                                                                                                                            \
        });                                                                                                                                                 \
    };                                                                                                                                                      \
    return asio::async_initiate<CompletionToken, void(RET)>(init, token, col, ARGS);                                                                        \
}


class IDBTaskBase;

class BASE_API UMongoAdapter final : public IDBAdapterBase {

public:
    UMongoAdapter();
    ~UMongoAdapter() override;

    void Initial(const IDataAsset_Interface *data) override;
    void Stop() override;

    IDBContext_Interface *AcquireContext() override;

    MONGO_OPERATE_DEFINE(InsertOne,
        ARGS_WRAPPER(const bsoncxx::document::value &doc, const mongocxx::options::insert &opt),
        ARGS_WRAPPER(doc, opt),
        std::optional<mongocxx::result::insert_one>, std::nullopt)

    MONGO_OPERATE_DEFINE(InsertMany,
        ARGS_WRAPPER(const std::vector<bsoncxx::document::value> &val, const mongocxx::options::insert &opt),
        ARGS_WRAPPER(val, opt),
        std::optional<mongocxx::result::insert_many>, std::nullopt)

    MONGO_OPERATE_DEFINE(UpdateOne,
        ARGS_WRAPPER(const bsoncxx::document::value &filter, const bsoncxx::document::value &doc, const mongocxx::options::update &opt),
        ARGS_WRAPPER(filter, doc, opt),
        std::optional<mongocxx::result::update>, std::nullopt)

    MONGO_OPERATE_DEFINE(UpdateMany,
        ARGS_WRAPPER(const bsoncxx::document::value &filter, const bsoncxx::document::value &doc, const mongocxx::options::update &opt),
        ARGS_WRAPPER(filter, doc, opt),
        std::optional<mongocxx::result::update>, std::nullopt)

    MONGO_OPERATE_DEFINE(Replace,
        ARGS_WRAPPER(const bsoncxx::document::value &filter, const bsoncxx::document::value &doc, const mongocxx::options::replace &opt),
        ARGS_WRAPPER(filter, doc, opt),
        std::optional<mongocxx::result::replace_one>, std::nullopt)

    MONGO_OPERATE_DEFINE(FindOne,
        ARGS_WRAPPER(const bsoncxx::document::value &filter, const mongocxx::options::find &opt),
        ARGS_WRAPPER(filter, opt),
        std::optional<bsoncxx::document::value>, std::nullopt)

    MONGO_OPERATE_DEFINE(Find,
        ARGS_WRAPPER(const bsoncxx::document::value &filter, const mongocxx::options::find &opt),
        ARGS_WRAPPER(filter, opt),
        std::optional<mongocxx::cursor>, std::nullopt)

    MONGO_OPERATE_DEFINE(DeleteOne,
        ARGS_WRAPPER(const bsoncxx::document::value &filter, const mongocxx::options::delete_options &opt),
        ARGS_WRAPPER(filter, opt),
        std::optional<mongocxx::result::delete_result>, std::nullopt)

    MONGO_OPERATE_DEFINE(DeleteMany,
        ARGS_WRAPPER(const bsoncxx::document::value &filter, const mongocxx::options::delete_options &opt),
        ARGS_WRAPPER(filter, opt),
        std::optional<mongocxx::result::delete_result>, std::nullopt)

    MONGO_OPERATE_DEFINE(Transaction,
        ARGS_WRAPPER(const mongocxx::client_session::with_transaction_cb &trans, const mongocxx::options::transaction &opt),
        ARGS_WRAPPER(trans, opt),
        int, EXIT_FAILURE)

private:
    mongocxx::instance mInstance;
    std::unique_ptr<mongocxx::pool> mPool;
    std::string mDatabaseName;

    std::atomic_bool bQuit;
};

#undef MONGO_OPERATE_DEFINE
