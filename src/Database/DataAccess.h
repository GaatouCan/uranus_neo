#pragma once

#include "Module.h"
#include "ConcurrentDeque.h"
#include "DBTask_Query.h"

#include <thread>
#include <vector>
#include <asio.hpp>
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

    template<class Callback>
    void PushQuery(const std::string &collection, const bsoncxx::document::value &document, Callback &&callback);

    template<asio::completion_token_for<void(std::optional<mongocxx::cursor>)> CompletionToken>
    auto AsyncQuery(const std::string &collection, const bsoncxx::document::value document, CompletionToken &&token) {
        auto init = [this](asio::completion_handler_for<void(std::optional<mongocxx::cursor>)> auto handle, const std::string &collection, const bsoncxx::document::value &document) {
            auto work = asio::make_work_guard(handle);

            if (mState != EModuleState::RUNNING) {
                auto alloc = asio::get_associated_allocator(handle, asio::recycling_allocator<void>());
                asio::dispatch(work.get_executor(), asio::bind_allocator(alloc, [handle = std::move(handle)]() mutable {
                    std::move(handle)(std::nullopt);
                }));
                return;
            }

            this->PushQuery(collection, document, [handle = std::move(handle), work = std::move(work)](std::optional<mongocxx::cursor> result) mutable {
                auto alloc = asio::get_associated_allocator(handle, asio::recycling_allocator<void>());
                asio::dispatch(work.get_executor(), asio::bind_allocator(alloc, [handle = std::move(handle), result = std::forward<std::optional<mongocxx::cursor>>(result)]() mutable {
                    std::move(handle)(std::move(result));
                }));
            });
        };

        return asio::async_initiate<CompletionToken, void(std::optional<mongocxx::cursor>)>(init, token, collection, document);
    };


private:
    mongocxx::instance mInstance;
    std::unique_ptr<mongocxx::pool> mPool;

    struct FWorkerNode {
        std::thread thread;
        TConcurrentDeque<std::unique_ptr<TDBTaskBase>, true> deque;
    };

    std::vector<FWorkerNode> mWorkerList;
    std::atomic_size_t mNextIndex;
};

template<class Callback>
void UDataAccess::PushQuery(const std::string &collection, const bsoncxx::document::value &document, Callback &&callback) {
    if (mState != EModuleState::RUNNING)
        return;

    if (mWorkerList.empty())
        return;

    auto &[thread, deque] = mWorkerList[mNextIndex++];
    mNextIndex = mNextIndex % mWorkerList.size();

    auto node = std::make_unique<UDBTask_Query<Callback>>(document, std::forward<Callback>(callback));
    node->SetCollectionName(collection);

    deque.PushBack(std::move(node));
}
