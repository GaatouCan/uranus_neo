#pragma once

#include "DBTaskBase.h"

#include <bsoncxx/builder/stream/document.hpp>
#include <utility>

template<class Callback>
class UDBTask_Query final : public TDBTaskBase {

    std::string mCollectionName;
    bsoncxx::document::value mDocument;

    Callback mCallback;

public:
    UDBTask_Query(bsoncxx::document::value queryDocument, Callback &&callback)
        : mDocument(std::move(queryDocument)),
          mCallback(std::forward<Callback>(callback)) {
    }

    ~UDBTask_Query() override = default;

    void SetCollectionName(const std::string &collectionName) {
        mCollectionName = collectionName;
    }

    void Execute(mongocxx::database &db) override {
        auto collection = db[mCollectionName];
        auto result = collection.find(mDocument.view());
        std::invoke(mCallback, std::make_optional(std::move(result)));
    }
};
