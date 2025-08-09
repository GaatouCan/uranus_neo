#pragma once

#include "Database/DBTaskBase.h"
#include "Database/Mongo/MongoContext.h"

#include <bsoncxx/document/value.hpp>
#include <mongocxx/options/replace.hpp>


namespace mongo {
    template<class Callback>
    class TDBTask_Replace final : public TDBTaskBase<Callback> {

        bsoncxx::document::value mFilter;
        bsoncxx::document::value mDocument;
        mongocxx::options::replace mOptions;

    public:
        TDBTask_Replace() = delete;

        TDBTask_Replace(
            std::string db,
            std::string col,
            Callback &&cb,
            bsoncxx::document::value filter,
            bsoncxx::document::value doc,
            mongocxx::options::replace opt = {}
        ): TDBTaskBase<Callback>(
               std::move(db),
               std::move(col),
               std::forward<Callback>(cb)
           ),
           mFilter(std::move(filter)),
           mDocument(std::move(doc)),
           mOptions(std::move(opt)) {
        }

        ~TDBTask_Replace() override = default;

        void Execute(IDBContext_Interface *ctx) override {
            const auto *context = dynamic_cast<FMongoContext *>(ctx);
            if (context == nullptr) {
                std::invoke(TDBTaskBase<Callback>::mCallback, std::nullopt);
                return;
            }

            auto db = context->entry[IDBTaskBase::mDatabase];
            auto collection = db[IDBTaskBase::mCollection];

            auto result = collection.replace_one(mFilter.view(), mDocument.view(), mOptions);
            std::invoke(TDBTaskBase<Callback>::mCallback, std::move(result));
        }
    };
}
