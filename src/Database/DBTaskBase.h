#pragma once

#include "Common.h"


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


template<class Callback = decltype(NoOperateCallback)>
class TDBTaskBase : public IDBTaskBase {
protected:
    Callback mCallback;

public:
    TDBTaskBase() = delete;
    ~TDBTaskBase() override = default;

    DISABLE_COPY_MOVE(TDBTaskBase)

    TDBTaskBase(std::string col, std::string db, Callback &&callback = NoOperateCallback)
        : IDBTaskBase(std::move(col), std::move(db)),
          mCallback(std::forward<Callback>(callback)) {
    }
};
