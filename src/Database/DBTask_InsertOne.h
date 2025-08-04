#pragma once

#include "DBTaskBase.h"

#include <bsoncxx/document/value.hpp>
#include <mongocxx/options/insert.hpp>


template<class Callback>
class BASE_API TDBTask_InsertOne final : public TDBTaskBase<Callback> {

    bsoncxx::document::value mDocument;
    mongocxx::options::insert mOptions;

public:
    TDBTask_InsertOne() = delete;

    TDBTask_InsertOne(
        std::string collection,
        Callback &&callback,
        bsoncxx::document::value document,
        mongocxx::options::insert options = {}
    ): TDBTaskBase<Callback>(std::move(collection), std::forward<Callback>(callback)),
       mDocument(std::move(document)),
       mOptions(std::move(options)) {
    }

    ~TDBTask_InsertOne() override = default;

    void Execute(mongocxx::client &client, mongocxx::database &db) override {
        auto collection = db[IDBTaskBase::mCollection];
        auto result = collection.insert_one(mDocument.view(), mOptions);
        std::invoke(TDBTaskBase<Callback>::mCallback, std::move(result));
    }
};
