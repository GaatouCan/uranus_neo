#pragma once

#include "Database/DBAdapterBase.h"

#include "DBTask_InsertOne.h"

#include <atomic>
#include <mongocxx/instance.hpp>
#include <mongocxx/pool.hpp>
#include <asio.hpp>


class IDBTaskBase;

class BASE_API UMongoAdapter final : public IDBAdapterBase {

public:
    UMongoAdapter();
    ~UMongoAdapter() override;

    void Initial(const IDataAsset_Interface *data) override;
    void Stop() override;

    IDBContext_Interface *AcquireContext() override;

    template<class Callback = decltype(NoOperateCallback)>
    void PushInsertOne(const std::string &col, const bsoncxx::document::value &doc, const mongocxx::options::insert &opt, Callback &&cb = NoOperateCallback) {
        auto task = std::make_unique<mongo::TDBTask_InsertOne<Callback>>(mDatabaseName, col, std::forward<Callback>(cb), doc, opt);
        this->PushTask(std::move(task));
    }
    template<asio::completion_token_for<void(std::optional<mongocxx::result::insert_one>)> CompletionToken>
    auto AsyncInsertOne(const std::string &col, const bsoncxx::document::value &doc, const mongocxx::options::insert &opt, CompletionToken &&token) {
        auto init = [this](asio::completion_handler_for<void(std::optional<mongocxx::result::insert_one>)> auto handle, const std::string &col, const bsoncxx::document::value &doc, const mongocxx::options::insert &opt) {
            auto work = asio::make_work_guard(handle);
            if (bQuit) {
                auto alloc = asio::get_associated_allocator(handle, asio::recycling_allocator<void>());
                asio::dispatch(work.get_executor(), asio::bind_allocator(alloc, [handle = std::move(handle)]() mutable {
                    std::move(handle)(std::nullopt);
                }));
                return;
            }
            this->PushInsertOne(col, doc, opt, [handle = std::move(handle), work = std::move(work)](std::optional<mongocxx::result::insert_one> result) mutable {
                auto alloc = asio::get_associated_allocator(handle, asio::recycling_allocator<void>());
                asio::dispatch(work.get_executor(), asio::bind_allocator(alloc, [handle = std::move(handle), result = std::forward<std::optional<mongocxx::result::insert_one>>(result)]() mutable {
                    std::move(handle)(std::move(result));
                }));
            });
        };
        return asio::async_initiate<CompletionToken, void(std::optional<mongocxx::result::insert_one>)>(init, token, col, doc, opt);
    };

private:
    mongocxx::instance mInstance;
    std::unique_ptr<mongocxx::pool> mPool;
    std::string mDatabaseName;

    std::atomic_bool bQuit;
};
