#pragma once

#include "DBTaskBase.h"

#include <bsoncxx/document/value.hpp>
#include <mongocxx/options/delete.hpp>


template<class Callback>
class BASE_API TDBTask_DeleteMany final : public TDBTaskBase<Callback> {

    bsoncxx::document::value mFilter;
    mongocxx::options::delete_options mOptions;

public:
    TDBTask_DeleteMany() = delete;

    TDBTask_DeleteMany(
        std::string collection,
        Callback &&callback,
        bsoncxx::document::value filter,
        mongocxx::options::delete_options options = {}
    ): TDBTaskBase<Callback>(std::move(collection), std::forward<Callback>(callback)),
       mFilter(std::move(filter)),
       mOptions(std::move(options)) {
    }

    ~TDBTask_DeleteMany() override = default;

    void Execute(mongocxx::client &client, mongocxx::database &db) override {
        auto collection = db[IDBTaskBase::mCollection];
        auto result = collection.delete_many(mFilter.view(),  mOptions);
        std::invoke(TDBTaskBase<Callback>::mCallback, std::move(result));
    }
};
