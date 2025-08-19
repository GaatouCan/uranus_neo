#pragma once

#include "Database/DBTaskBase.h"
#include "MongoContext.h"

#include <vector>
#include <optional>
#include <bsoncxx/document/value.hpp>
#include <mongocxx/options/find.hpp>
#include <mongocxx/options/insert.hpp>
#include <mongocxx/options/replace.hpp>
#include <mongocxx/options/update.hpp>
#include <mongocxx/options/delete.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/client_session.hpp>
#include <mongocxx/options/transaction.hpp>


namespace mongo {
    template<class Callback>
    class TDBTask_Find final : public TDBTaskBase<Callback> {

        bsoncxx::document::value mFilter;
        mongocxx::options::find mOptions;

    public:
        TDBTask_Find() = delete;

        TDBTask_Find(
            std::string db,
            std::string col,
            Callback &&cb,
            bsoncxx::document::value filter,
            mongocxx::options::find options
        ): TDBTaskBase<Callback>(
               std::move(db),
               std::move(col),
               std::forward<Callback>(cb)
           ),
           mFilter(std::move(filter)),
           mOptions(std::move(options)) {
        }

        ~TDBTask_Find() override = default;

        void Execute(IDBContext_Interface *ctx) override {
            const auto *context = dynamic_cast<FMongoContext *>(ctx);
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

    template<class Callback>
    class TDBTask_InsertMany final : public TDBTaskBase<Callback> {
        std::vector<bsoncxx::document::value> mValues;
        mongocxx::options::insert mOptions;

    public:
        TDBTask_InsertMany() = delete;

        TDBTask_InsertMany(
            std::string db,
            std::string col,
            Callback &&cb,
            std::vector<bsoncxx::document::value> val,
            mongocxx::options::insert opt = {}
        ): TDBTaskBase<Callback>(
               std::move(db),
               std::move(col),
               std::forward<Callback>(cb)),
           mValues(std::move(val)),
           mOptions(std::move(opt)) {
        }

        ~TDBTask_InsertMany() override = default;

        void Execute(IDBContext_Interface *ctx) override {
            const auto *context = dynamic_cast<FMongoContext *>(ctx);
            if (context == nullptr) {
                std::invoke(TDBTaskBase<Callback>::mCallback, std::nullopt);
                return;
            }

            auto db = context->entry[IDBTaskBase::mDatabase];
            auto collection = db[IDBTaskBase::mCollection];

            auto result = collection.insert_many(mValues, mOptions);
            std::invoke(TDBTaskBase<Callback>::mCallback, std::move(result));
        }
    };

    template<class Callback>
    class TDBTask_InsertOne final : public TDBTaskBase<Callback> {
        bsoncxx::document::value mDocument;
        mongocxx::options::insert mOptions;

    public:
        TDBTask_InsertOne() = delete;

        TDBTask_InsertOne(
            std::string db,
            std::string col,
            Callback &&cb,
            bsoncxx::document::value doc,
            mongocxx::options::insert opt = {}
        ): TDBTaskBase<Callback>(
               std::move(db),
               std::move(col),
               std::forward<Callback>(cb)
           ),
           mDocument(std::move(doc)),
           mOptions(std::move(opt)) {
        }

        ~TDBTask_InsertOne() override = default;

        void Execute(IDBContext_Interface *ctx) override {
            const auto *context = dynamic_cast<FMongoContext *>(ctx);
            if (context == nullptr) {
                std::invoke(TDBTaskBase<Callback>::mCallback, std::nullopt);
                return;
            }

            auto db = context->entry[IDBTaskBase::mDatabase];
            auto collection = db[IDBTaskBase::mCollection];

            auto result = collection.insert_one(mDocument.view(), mOptions);
            std::invoke(TDBTaskBase<Callback>::mCallback, std::move(result));
        }
    };

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

    template<class Callback>
    class TDBTask_UpdateMany final : public TDBTaskBase<Callback> {

        bsoncxx::document::value mFilter;
        bsoncxx::document::value mDocument;
        mongocxx::options::update mOptions;

    public:
        TDBTask_UpdateMany() = delete;

        TDBTask_UpdateMany(
            std::string db,
            std::string col,
            Callback &&cb,
            bsoncxx::document::value filter,
            bsoncxx::document::value doc,
            mongocxx::options::update opt = {}
        ): TDBTaskBase<Callback>(
               std::move(db),
               std::move(col),
               std::forward<Callback>(cb)
           ),
           mFilter(std::move(filter)),
           mDocument(std::move(doc)),
           mOptions(std::move(opt)) {
        }

        ~TDBTask_UpdateMany() override = default;

        void Execute(IDBContext_Interface *ctx) override {
            const auto *context = dynamic_cast<FMongoContext *>(ctx);
            if (context == nullptr) {
                std::invoke(TDBTaskBase<Callback>::mCallback, std::nullopt);
                return;
            }

            auto db = context->entry[IDBTaskBase::mDatabase];
            auto collection = db[IDBTaskBase::mCollection];

            auto result = collection.update_many(mFilter.view(), mDocument.view(), mOptions);
            std::invoke(TDBTaskBase<Callback>::mCallback, std::move(result));
        }
    };

    template<class Callback>
    class TDBTask_UpdateOne final : public TDBTaskBase<Callback> {

        bsoncxx::document::value mFilter;
        bsoncxx::document::value mDocument;
        mongocxx::options::update mOptions;

    public:
        TDBTask_UpdateOne() = delete;

        TDBTask_UpdateOne(
            std::string db,
            std::string col,
            Callback &&cb,
            bsoncxx::document::value filter,
            bsoncxx::document::value doc,
            mongocxx::options::update opt = {}
        ): TDBTaskBase<Callback>(
               std::move(db),
               std::move(col),
               std::forward<Callback>(cb)
           ),
           mFilter(std::move(filter)),
           mDocument(std::move(doc)),
           mOptions(std::move(opt)) {
        }

        ~TDBTask_UpdateOne() override = default;

        void Execute(IDBContext_Interface *ctx) override {
            const auto *context = dynamic_cast<FMongoContext *>(ctx);
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

    template<class Callback>
    class TDBTask_DeleteMany final : public TDBTaskBase<Callback> {

        bsoncxx::document::value mFilter;
        mongocxx::options::delete_options mOptions;

    public:
        TDBTask_DeleteMany() = delete;

        TDBTask_DeleteMany(
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

        ~TDBTask_DeleteMany() override = default;

        void Execute(IDBContext_Interface *ctx) override {
            const auto *context = dynamic_cast<FMongoContext *>(ctx);
            if (context == nullptr) {
                std::invoke(TDBTaskBase<Callback>::mCallback, std::nullopt);
                return;
            }

            auto db = context->entry[IDBTaskBase::mDatabase];
            auto collection = db[IDBTaskBase::mCollection];

            auto result = collection.delete_many(mFilter.view(), mOptions);
            std::invoke(TDBTaskBase<Callback>::mCallback, std::move(result));
        }
    };

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

    template<class Callback>
    class TDBTask_Transaction final : public TDBTaskBase<Callback> {

        mongocxx::client_session::with_transaction_cb mTransaction;
        mongocxx::options::transaction mOptions;

    public:
        TDBTask_Transaction() = delete;

        TDBTask_Transaction(
            std::string db,
            std::string col,
            Callback &&cb,
            mongocxx::client_session::with_transaction_cb trans,
            mongocxx::options::transaction opt = {}
        ): TDBTaskBase<Callback>(
               std::move(db),
               std::move(col),
               std::forward<Callback>(cb)
           ),
           mTransaction(std::move(trans)),
           mOptions(std::move(opt)) {
        }

        ~TDBTask_Transaction() override = default;

        void Execute(IDBContext_Interface *ctx) override {
            auto *context = dynamic_cast<FMongoContext *>(ctx);
            if (context == nullptr) {
                std::invoke(TDBTaskBase<Callback>::mCallback, std::nullopt);
                return;
            }

            auto session = context->entry->start_session();
            try {
                session.with_transaction(mTransaction, mOptions);
            } catch (const std::exception &e) {
                std::invoke(TDBTask_Transaction::mCallback, std::move(EXIT_FAILURE));
                return;
            }
            std::invoke(TDBTask_Transaction::mCallback, std::move(EXIT_SUCCESS));
        }
    };
}
