#pragma once

#include "Database/DBTaskBase.h"
#include "Database/Mongo/MongoContext.h"

#include <vector>
#include <bsoncxx/document/value.hpp>
#include <mongocxx/options/insert.hpp>

namespace mongo {
    template<class Callback>
    class TDBTask_InsertMany final : public TDBTaskBase<Callback> {

        std::vector<bsoncxx::document::value> mValues;
        mongocxx::options::insert mOptions;

    public:
        TDBTask_InsertMany() = delete;

        TDBTask_InsertMany(
            std::string collection,
            Callback &&callback,
            std::vector<bsoncxx::document::value> values,
            mongocxx::options::insert options = {}
        ): TDBTaskBase<Callback>(std::move(collection), std::forward<Callback>(callback)),
           mValues(std::move(values)),
           mOptions(std::move(options)) {
        }

        ~TDBTask_InsertMany() override = default;

        void Execute(IDBContext_Interface *ctx) override {
            auto *context = dynamic_cast<FMongoContext *>(ctx);
            if (context == nullptr) {
                std::invoke(TDBTaskBase<Callback>::mCallback, std::nullopt);
                return;
            }

            auto db = context->mEntry[IDBTaskBase::mDatabase];
            auto collection = db[IDBTaskBase::mCollection];
            auto result = collection.insert_many(mValues, mOptions);
            std::invoke(TDBTaskBase<Callback>::mCallback, std::move(result));
        }
    };
}
