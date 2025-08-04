#pragma once

#include "DBTaskBase.h"

#include <bsoncxx/document/value.hpp>
#include <mongocxx/options/replace.hpp>


template<class Callback>
class BASE_API TDBTask_Replace final : public TDBTaskBase<Callback> {

    bsoncxx::document::value mFilter;
    bsoncxx::document::value mDocument;
    mongocxx::options::replace mOptions;

public:
    TDBTask_Replace() = delete;

    TDBTask_Replace(
        std::string collection,
        Callback &&callback,
        bsoncxx::document::value filter,
        bsoncxx::document::value document,
        mongocxx::options::replace options = {}
    ): TDBTaskBase<Callback>(std::move(collection), std::forward<Callback>(callback)),
       mFilter(std::move(filter)),
       mDocument(std::move(document)),
       mOptions(std::move(options)) {
    }

    ~TDBTask_Replace() override = default;

    void Execute(mongocxx::client &client, mongocxx::database &db) override {
        auto collection = db[IDBTaskBase::mCollection];
        auto result = collection.replace_one(mFilter.view(), mDocument.view(), mOptions);
        std::invoke(TDBTaskBase<Callback>::mCallback, std::move(result));
    }
};
