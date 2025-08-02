#pragma once

#include "DatabaseTask.h"

#include <bsoncxx/builder/stream/document.hpp>
#include <utility>

template<class Callback>
class UDBTask_QueryOne final : public IDatabaseTask_Interface {

    std::string mCollectionName;
    bsoncxx::document::value mDocument;

    Callback mCallback;

public:
    UDBTask_QueryOne(bsoncxx::document::value queryDocument, Callback &&callback)
        : mDocument(std::move(queryDocument)),
          mCallback(std::forward<Callback>(callback)) {
    }

    ~UDBTask_QueryOne() override = default;

    void SetCollectionName(const std::string &collectionName) {
        mCollectionName = collectionName;
    }

    void Execute(mongocxx::database &db) override {
        auto collection = db[mCollectionName];
        auto result = collection.find_one(mDocument.view());
        std::invoke(mCallback, std::make_optional(std::move(result)));
    }
};
