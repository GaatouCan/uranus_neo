#pragma once

#include "Common.h"

#include <string>


struct IDBContext_Interface;


class BASE_API IDBTaskBase {

protected:
    std::string mDatabase;
    std::string mCollection;

public:
    IDBTaskBase() = default;
    virtual ~IDBTaskBase() = default;

    DISABLE_COPY_MOVE(IDBTaskBase)

    IDBTaskBase(std::string db, std::string col)
        : mDatabase(std::move(db)),
          mCollection(std::move(col)) {
    }

    void SetDatabase(const std::string &database) { mDatabase = database; }
    [[nodiscard]] const std::string &GetDatabaseName() const { return mDatabase; }

    void SetCollection(const std::string &collection) { mCollection = collection; }
    [[nodiscard]] const std::string &GetCollectionName() const { return mCollection; }

    virtual void Execute(IDBContext_Interface *context) = 0;
};


template<class Callback = decltype(EmptyCallback)>
class TDBTaskBase : public IDBTaskBase {
protected:
    Callback mCallback;

public:
    TDBTaskBase() = delete;
    ~TDBTaskBase() override = default;

    DISABLE_COPY_MOVE(TDBTaskBase)

    TDBTaskBase(std::string db, std::string col, Callback &&callback = EmptyCallback)
        : IDBTaskBase(std::move(db), std::move(col)),
          mCallback(std::forward<Callback>(callback)) {
    }
};
