#pragma once

#include "DBTaskBase.h"

#include <bsoncxx/document/value.hpp>
#include <mongocxx/options/update.hpp>


template<class Callback>
class BASE_API TDBTask_UpdateOne final : public TDBTaskBase<Callback> {

    bsoncxx::document::value mFilter;
    bsoncxx::document::value mDocument;
    mongocxx::options::update mOptions;

public:
    TDBTask_UpdateOne() = delete;

    TDBTask_UpdateOne(
        std::string collection,
        Callback &&callback,
        bsoncxx::document::value filter,
        bsoncxx::document::value document,
        mongocxx::options::update options = {}
    ): TDBTaskBase<Callback>(std::move(collection), std::forward<Callback>(callback)),
       mFilter(std::move(filter)),
       mDocument(std::move(document)),
       mOptions(std::move(options)) {
    }

    ~TDBTask_UpdateOne() override = default;

    void Execute(mongocxx::client &client, mongocxx::database &db) override {
        auto collection = db[IDBTaskBase::mCollection];
        auto result = collection.update_one(mFilter.view(), mDocument.view(), mOptions);
        std::invoke(TDBTaskBase<Callback>::mCallback, std::move(result));
    }
};
