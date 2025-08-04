#pragma once

#include "DBTaskBase.h"

#include <bsoncxx/document/value.hpp>
#include <optional>

template<class Callback>
class BASE_API TDBTask_FindOne final : public TDBTaskBase<Callback> {

    bsoncxx::document::value mQueryFilter;

public:
    TDBTask_FindOne() = delete;

    TDBTask_FindOne(
        std::string collection,
        Callback &&callback,
        bsoncxx::document::value filter
    ): TDBTaskBase<Callback>(std::move(collection), std::forward<Callback>(callback)),
       mQueryFilter(std::move(filter)) {
    }

    ~TDBTask_FindOne() override = default;

    void Execute(mongocxx::client &client, mongocxx::database &db) override {
        auto collection = db[IDBTaskBase::mCollection];
        auto result = collection.find_one(mQueryFilter.view());
        std::invoke(TDBTaskBase<Callback>::mCallback, std::move(result));
    }
};
