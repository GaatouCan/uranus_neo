#include "DBAdapterBase.h"

#include "DataAccess.h"

IDBAdapterBase::IDBAdapterBase()
    : mDataAccess(nullptr) {
}

IDBAdapterBase::~IDBAdapterBase() {
}

void IDBAdapterBase::SetUpModule(UDataAccess *module) {
    mDataAccess = module;
}

UDataAccess *IDBAdapterBase::GetDataAccess() const {
    return mDataAccess;
}

void IDBAdapterBase::Initial(const IDataAsset_Interface *data) {
}

void IDBAdapterBase::Stop() {
}

void IDBAdapterBase::PushTask(std::unique_ptr<IDBTaskBase> &&task) const {
    if (mDataAccess) {
        mDataAccess->PushTask(std::move(task));
    }
}
