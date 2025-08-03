#pragma once

#include "DBTaskBase.h"

#include <bsoncxx/document/value.hpp>
#include <optional>

template<class Callback>
class BASE_API TDBTask_Find final : public TDBTaskBase<Callback> {

    bsoncxx::document::value mQueryFilter;

public:
    TDBTask_Find() = delete;

    TDBTask_Find(
        std::string collection,
        Callback &&callback,
        bsoncxx::document::value filter
    ): TDBTaskBase<Callback>(std::move(collection), std::forward<Callback>(callback)),
       mQueryFilter(std::move(filter)) {
    }

    ~TDBTask_Find() override = default;

    void Execute(mongocxx::database &db) override {
        auto collection = db[IDBTaskBase::mCollection];
        auto result = collection.find(mQueryFilter.view());
        std::invoke(TDBTaskBase<Callback>::mCallback, std::make_optional(std::move(result)));
    }
};
