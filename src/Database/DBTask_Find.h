#pragma once

#include "DBTaskBase.h"

#include <bsoncxx/document/value.hpp>
#include <mongocxx/options/find.hpp>
#include <optional>

template<class Callback>
class BASE_API TDBTask_Find final : public TDBTaskBase<Callback> {

    bsoncxx::document::value mFilter;
    mongocxx::options::find mOptions;

public:
    TDBTask_Find() = delete;

    TDBTask_Find(
        std::string collection,
        Callback &&callback,
        bsoncxx::document::value filter,
        mongocxx::options::find options
    ): TDBTaskBase<Callback>(std::move(collection), std::forward<Callback>(callback)),
       mFilter(std::move(filter)),
       mOptions(std::move(options)) {
    }

    ~TDBTask_Find() override = default;

    void Execute(mongocxx::client &client, mongocxx::database &db) override {
        auto collection = db[IDBTaskBase::mCollection];
        auto result = collection.find(mFilter.view(), mOptions);
        std::invoke(TDBTaskBase<Callback>::mCallback, std::make_optional(std::move(result)));
    }
};
