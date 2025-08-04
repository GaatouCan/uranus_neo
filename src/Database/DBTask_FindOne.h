#pragma once

#include "DBTaskBase.h"

#include <bsoncxx/document/value.hpp>
#include <mongocxx/options/find.hpp>

template<class Callback>
class BASE_API TDBTask_FindOne final : public TDBTaskBase<Callback> {

    bsoncxx::document::value mFilter;
    mongocxx::options::find mOptions;

public:
    TDBTask_FindOne() = delete;

    TDBTask_FindOne(
        std::string collection,
        Callback &&callback,
        bsoncxx::document::value filter,
        mongocxx::options::find options
    ): TDBTaskBase<Callback>(std::move(collection), std::forward<Callback>(callback)),
       mFilter(std::move(filter)),
       mOptions(std::move(options)) {
    }

    ~TDBTask_FindOne() override = default;

    void Execute(mongocxx::client &client, mongocxx::database &db) override {
        auto collection = db[IDBTaskBase::mCollection];
        auto result = collection.find_one(mFilter.view(), mOptions);
        std::invoke(TDBTaskBase<Callback>::mCallback, std::move(result));
    }
};
