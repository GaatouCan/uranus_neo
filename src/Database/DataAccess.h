#pragma once

#include "Module.h"
#include "ConcurrentDeque.h"

#include "DBTask_Find.h"
#include "DBTask_FindOne.h"
#include "DBTask_InsertOne.h"
#include "DBTask_InsertMany.h"
#include "DBTask_UpdateOne.h"
#include "DBTask_UpdateMany.h"
#include "DBTask_Replace.h"
#include "DBTask_DeleteOne.h"
#include "DBTask_DeleteMany.h"
#include "DBTask_Transaction.h"

#include <thread>
#include <vector>
#include <asio.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/pool.hpp>


#define DATABASE_OPERATION_PARAMS(...)       __VA_ARGS__
#define DATABASE_OPERATION_CALL_ARGS(...)    __VA_ARGS__

/** Database Operation Async Helper Macro **/
#define DEFINE_DATABASE_OPERATION(XX, PARAMS, CALL_ARGS, RETURN, RETURN_FAIL)                                                                                       \
    template<class Callback>                                                                                                                                        \
    void Push##XX(const std::string &collection, PARAMS, Callback &&callback) {                                                                                     \
        if (mState != EModuleState::RUNNING) return;                                                                                                                \
        if (mWorkerList.empty()) return;                                                                                                                            \
        auto node = std::make_unique<TDBTask_##XX<Callback>>(collection, std::forward<Callback>(callback), CALL_ARGS);                                              \
        this->PushTask(std::move(node));                                                                                                                            \
    }                                                                                                                                                               \
    template<asio::completion_token_for<void(RETURN)> CompletionToken>                                                                                              \
    auto Async##XX(const std::string &collection, PARAMS, CompletionToken &&token) {                                                                                \
        auto init = [this](asio::completion_handler_for<void(RETURN)> auto handle, const std::string &collection, PARAMS) {                                         \
            auto work = asio::make_work_guard(handle);                                                                                                              \
            if (mState != EModuleState::RUNNING) {                                                                                                                  \
                auto alloc = asio::get_associated_allocator(handle, asio::recycling_allocator<void>());                                                             \
                asio::dispatch(work.get_executor(), asio::bind_allocator(alloc, [handle = std::move(handle)]() mutable {                                            \
                    std::move(handle)(RETURN_FAIL);                                                                                                                 \
                }));                                                                                                                                                \
                return;                                                                                                                                             \
            }                                                                                                                                                       \
            this->Push##XX(collection, CALL_ARGS, [handle = std::move(handle), work = std::move(work)](RETURN result) mutable {                                     \
                auto alloc = asio::get_associated_allocator(handle, asio::recycling_allocator<void>());                                                             \
                asio::dispatch(work.get_executor(), asio::bind_allocator(alloc, [handle = std::move(handle), result = std::forward<RETURN>(result)]() mutable {     \
                    std::move(handle)(std::move(result));                                                                                                           \
                }));                                                                                                                                                \
            });                                                                                                                                                     \
        };                                                                                                                                                          \
        return asio::async_initiate<CompletionToken, void(RETURN)>(init, token, collection, CALL_ARGS);                                                             \
    };


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

    mongocxx::cursor SyncSelect(const std::string &collection, const bsoncxx::document::value &filter) const;

    // DEFINE_DATABASE_OPERATION(FindOne,
    //     DATABASE_OPERATION_PARAMS(const bsoncxx::document::value &document, const mongocxx::options::find &options),
    //     DATABASE_OPERATION_CALL_ARGS(document, options),
    //     std::optional<bsoncxx::document::value>, std::nullopt)
    //
    // DEFINE_DATABASE_OPERATION(Find,
    //     DATABASE_OPERATION_PARAMS(const bsoncxx::document::value &document, const mongocxx::options::find &options),
    //     DATABASE_OPERATION_CALL_ARGS(document, options),
    //     std::optional<mongocxx::cursor>, std::nullopt)
    //
    // DEFINE_DATABASE_OPERATION(InsertOne,
    //     DATABASE_OPERATION_PARAMS(const bsoncxx::document::value &document, const mongocxx::options::insert &options),
    //     DATABASE_OPERATION_CALL_ARGS(document, options),
    //     std::optional<mongocxx::result::insert_one>, std::nullopt)
    //
    // DEFINE_DATABASE_OPERATION(InsertMany,
    //     DATABASE_OPERATION_PARAMS(const bsoncxx::document::value &document, const mongocxx::options::insert &options),
    //     DATABASE_OPERATION_CALL_ARGS(document, options),
    //     std::optional<mongocxx::result::insert_many>, std::nullopt)
    //
    // DEFINE_DATABASE_OPERATION(UpdateOne,
    //     DATABASE_OPERATION_PARAMS(const bsoncxx::document::value &filter, const bsoncxx::document::value &document, const mongocxx::options::update &options),
    //     DATABASE_OPERATION_CALL_ARGS(filter, document, options),
    //     std::optional<mongocxx::result::update>, std::nullopt)
    //
    // DEFINE_DATABASE_OPERATION(UpdateMany,
    //     DATABASE_OPERATION_PARAMS(const bsoncxx::document::value &filter, const bsoncxx::document::value &document, const mongocxx::options::update &options),
    //     DATABASE_OPERATION_CALL_ARGS(filter, document, options),
    //     std::optional<mongocxx::result::update>, std::nullopt)
    //
    // DEFINE_DATABASE_OPERATION(Replace,
    //     DATABASE_OPERATION_PARAMS(const bsoncxx::document::value &filter, const bsoncxx::document::value &document, const mongocxx::options::replace &options),
    //     DATABASE_OPERATION_CALL_ARGS(filter, document, options),
    //     std::optional<mongocxx::result::replace_one>, std::nullopt)
    //
    // DEFINE_DATABASE_OPERATION(DeleteOne,
    //     DATABASE_OPERATION_PARAMS(const bsoncxx::document::value &filter, const mongocxx::options::delete_options &options),
    //     DATABASE_OPERATION_CALL_ARGS(filter, options),
    //     std::optional<mongocxx::result::delete_result>, std::nullopt)
    //
    // DEFINE_DATABASE_OPERATION(DeleteMany,
    //     DATABASE_OPERATION_PARAMS(const bsoncxx::document::value &filter, const mongocxx::options::delete_options &options),
    //     DATABASE_OPERATION_CALL_ARGS(filter, options),
    //     std::optional<mongocxx::result::delete_result>, std::nullopt)
    //
    // DEFINE_DATABASE_OPERATION(Transaction,
    //     DATABASE_OPERATION_PARAMS(const mongocxx::client_session::with_transaction_cb &transaction, const mongocxx::options::transaction &options),
    //     DATABASE_OPERATION_CALL_ARGS(transaction, options),
    //     int, 0)

private:
    void PushTask(std::unique_ptr<IDBTaskBase> task);

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


#undef DATABASE_OPERATION_PARAMS
#undef DATABASE_OPERATION_CALL_ARGS
#undef DEFINE_DATABASE_OPERATION
