#pragma once

#include "Database/DBTaskBase.h"
#include "Database/Mongo/MongoContext.h"

#include <bsoncxx/document/value.hpp>
#include <mongocxx/options/update.hpp>


namespace mongo {
    template<class Callback>
    class TDBTask_UpdateOne final : public TDBTaskBase<Callback> {

        bsoncxx::document::value mFilter;
        bsoncxx::document::value mDocument;
        mongocxx::options::update mOptions;

    public:
        TDBTask_UpdateOne() = delete;

        TDBTask_UpdateOne(
            std::string collection,
            Callback &&callback,
            bsoncxx::document::value filter,
            bsoncxx::document::value document,
            mongocxx::options::update options = {}
        ): TDBTaskBase<Callback>(std::move(collection), std::forward<Callback>(callback)),
           mFilter(std::move(filter)),
           mDocument(std::move(document)),
           mOptions(std::move(options)) {
        }

        ~TDBTask_UpdateOne() override = default;

        void Execute(IDBContext_Interface *ctx) override {
            auto *context = dynamic_cast<FMongoContext *>(ctx);
            if (context == nullptr) {
                std::invoke(TDBTaskBase<Callback>::mCallback, std::nullopt);
                return;
            }

            auto db = context->entry[IDBTaskBase::mDatabase];
            auto collection = db[IDBTaskBase::mCollection];
            auto result = collection.update_one(mFilter.view(), mDocument.view(), mOptions);
            std::invoke(TDBTaskBase<Callback>::mCallback, std::move(result));
        }
    };
}
