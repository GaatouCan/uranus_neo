#pragma once

#include "DatabaseTask.h"

#include <bsoncxx/builder/stream/document.hpp>


template<class Callback>
class UDatabase_Query final : public IDatabaseTask_Interface {

    std::string mCollectionName;
    bsoncxx::document::value mDocument;

    Callback mCallback;

public:
    UDatabase_Query(const bsoncxx::document::value &queryDocument, Callback &&callback)
        : mDocument(queryDocument),
          mCallback(std::forward<Callback>(callback)) {
    }

    ~UDatabase_Query() override = default;

    void SetCollectionName(const std::string &collectionName) {
        mCollectionName = collectionName;
    }

    void Execute(mongocxx::database &db) override {
        auto collection = db[mCollectionName];
        // auto result = std::make_shared<mongocxx::cursor>(collection.find(mDocument.view()));
        auto result = collection.find(mDocument.view());
        std::invoke(mCallback, std::make_optional(std::move(result)));
    }
};
