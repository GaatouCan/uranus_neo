#pragma once

#include "Common.h"

#include <mongocxx/database.hpp>
#include <utility>

class BASE_API IDBTaskBase {

protected:
    std::string mCollection;

public:
    IDBTaskBase() = default;
    virtual ~IDBTaskBase() = default;

    DISABLE_COPY_MOVE(IDBTaskBase)

    explicit IDBTaskBase(std::string collection)
        : mCollection(std::move(collection)) {
    }

    void SetCollection(const std::string &collection) { mCollection = collection; }
    [[nodiscard]] const std::string &GetCollectionName() const { return mCollection; }

    virtual void Execute(mongocxx::database &db) = 0;
};


template<class Callback>
class TDBTaskBase : public IDBTaskBase {

protected:
    Callback mCallback;

public:
    TDBTaskBase() = default;
    ~TDBTaskBase() override = default;

    DISABLE_COPY_MOVE(TDBTaskBase)

    TDBTaskBase(std::string collection, Callback &&callback)
        : IDBTaskBase(std::move(collection)),
          mCallback(std::forward<Callback>(callback)) {
    }

    void SetCallback(Callback &&callback) { mCallback = std::forward<Callback>(callback); }
};
