#pragma once

#include "Database/DBTaskBase.h"
#include "Database/Mongo/MongoContext.h"

#include <bsoncxx/document/value.hpp>
#include <mongocxx/options/find.hpp>


namespace mongo {
    template<class Callback>
    class TDBTask_FindOne final : public TDBTaskBase<Callback> {

        bsoncxx::document::value mFilter;
        mongocxx::options::find mOptions;

    public:
        TDBTask_FindOne() = delete;

        TDBTask_FindOne(
            std::string db,
            std::string col,
            Callback &&cb,
            bsoncxx::document::value filter,
            mongocxx::options::find opt
        ): TDBTaskBase<Callback>(
               std::move(db),
               std::move(col),
               std::forward<Callback>(cb)
           ),
           mFilter(std::move(filter)),
           mOptions(std::move(opt)) {
        }

        ~TDBTask_FindOne() override = default;

        void Execute(IDBContext_Interface *ctx) override {
            const auto *context = dynamic_cast<FMongoContext *>(ctx);
            if (context == nullptr) {
                std::invoke(TDBTaskBase<Callback>::mCallback, std::nullopt);
                return;
            }

            auto db = context->entry[IDBTaskBase::mDatabase];
            auto collection = db[IDBTaskBase::mCollection];

            auto result = collection.find_one(mFilter.view(), mOptions);
            std::invoke(TDBTaskBase<Callback>::mCallback, std::move(result));
        }
    };
}
