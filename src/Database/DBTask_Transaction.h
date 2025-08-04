#pragma once

#include "DBTaskBase.h"

#include <mongocxx/client.hpp>
#include <mongocxx/client_session.hpp>
#include <mongocxx/options/transaction.hpp>

template<class Callback>
class BASE_API TDBTask_Transaction final : public TDBTaskBase<Callback> {

    mongocxx::client_session::with_transaction_cb mTransaction;
    mongocxx::options::transaction mOptions;

public:
    TDBTask_Transaction() = delete;

    TDBTask_Transaction(
        std::string collection,
        Callback &&callback,
        mongocxx::client_session::with_transaction_cb transaction,
        mongocxx::options::transaction options = {}
    ): TDBTaskBase<Callback>(std::move(collection), std::forward<Callback>(callback)),
       mTransaction(std::move(transaction)),
       mOptions(std::move(options)) {
    }

    ~TDBTask_Transaction() override = default;

    void Execute(mongocxx::client &client, mongocxx::database &db) override {
        auto session = client.start_session();
        try {
            session.with_transaction(mTransaction, mOptions);
        } catch (const std::exception &e) {
            std::invoke(TDBTask_Transaction::mCallback, std::move(EXIT_FAILURE));
            return;
        }
        std::invoke(TDBTask_Transaction::mCallback, std::move(EXIT_SUCCESS));
    }
};

