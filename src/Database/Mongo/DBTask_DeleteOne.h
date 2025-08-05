#pragma once

#include "Database/DBTaskBase.h"
#include "Database/Mongo/MongoContext.h"

#include <bsoncxx/document/value.hpp>
#include <mongocxx/options/delete.hpp>


namespace mongo {
    template<class Callback>
    class TDBTask_DeleteOne final : public TDBTaskBase<Callback> {

        bsoncxx::document::value mFilter;
        mongocxx::options::delete_options mOptions;

    public:
        TDBTask_DeleteOne() = delete;

        TDBTask_DeleteOne(
            std::string db,
            std::string col,
            Callback &&cb,
            bsoncxx::document::value filter,
            mongocxx::options::delete_options opt = {}
        ): TDBTaskBase<Callback>(
               std::move(db),
               std::move(col),
               std::forward<Callback>(cb)
           ),
           mFilter(std::move(filter)),
           mOptions(std::move(opt)) {
        }

        ~TDBTask_DeleteOne() override = default;

        void Execute(IDBContext_Interface *ctx) override {
            const auto *context = dynamic_cast<FMongoContext *>(ctx);
            if (context == nullptr) {
                std::invoke(TDBTaskBase<Callback>::mCallback, std::nullopt);
                return;
            }

            auto db = context->entry[IDBTaskBase::mDatabase];
            auto collection = db[IDBTaskBase::mCollection];

            auto result = collection.delete_one(mFilter.view(), mOptions);
            std::invoke(TDBTaskBase<Callback>::mCallback, std::move(result));
        }
    };
}
