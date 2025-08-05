#pragma once

#include "Database/DBTaskBase.h"
#include "Database/Mongo/MongoContext.h"

#include <bsoncxx/document/value.hpp>
#include <mongocxx/options/find.hpp>
#include <optional>


namespace mongo {
    template<class Callback>
    class TDBTask_Find final : public TDBTaskBase<Callback> {

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

        void Execute(IDBContext_Interface *ctx) override {
            auto *context = dynamic_cast<FMongoContext *>(ctx);
            if (context == nullptr) {
                std::invoke(TDBTaskBase<Callback>::mCallback, std::nullopt);
                return;
            }

            auto db = context->entry[IDBTaskBase::mDatabase];
            auto collection = db[IDBTaskBase::mCollection];
            auto result = collection.find(mFilter.view(), mOptions);
            std::invoke(TDBTaskBase<Callback>::mCallback, std::make_optional(std::move(result)));
        }
    };
}
