#pragma once

#include "Database/DBTaskBase.h"
#include "Database/Mongo/MongoContext.h"

#include <bsoncxx/document/value.hpp>
#include <mongocxx/options/insert.hpp>

namespace mongo {
    template<class Callback>
    class TDBTask_InsertOne final : public TDBTaskBase<Callback> {
        bsoncxx::document::value mDocument;
        mongocxx::options::insert mOptions;

    public:
        TDBTask_InsertOne() = delete;

        TDBTask_InsertOne(
            std::string collection,
            Callback &&callback,
            bsoncxx::document::value document,
            mongocxx::options::insert options = {}
        ): TDBTaskBase<Callback>(std::move(collection), std::forward<Callback>(callback)),
           mDocument(std::move(document)),
           mOptions(std::move(options)) {
        }

        ~TDBTask_InsertOne() override = default;

        void Execute(IDBContextBase *ctx) override {
            auto *context = dynamic_cast<FMongoContext *>(ctx);
            if (context == nullptr) {
                std::invoke(TDBTaskBase<Callback>::mCallback, std::nullopt);
                return;
            }

            auto db = context->mEntry[IDBTaskBase::mDatabase];
            auto collection = db[IDBTaskBase::mCollection];
            auto result = collection.insert_one(mDocument.view(), mOptions);
            std::invoke(TDBTaskBase<Callback>::mCallback, std::move(result));
        }
    };
}
