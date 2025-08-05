#include "DBAdapterBase.h"

IDBAdapterBase::IDBAdapterBase()
    : mOwner(nullptr) {
}

IDBAdapterBase::~IDBAdapterBase() {
}

void IDBAdapterBase::SetUpModule(UDataAccess *module) {
    mOwner = module;
}

UDataAccess *IDBAdapterBase::GetDataAccess() const {
    return mOwner;
}

void IDBAdapterBase::Initial(const IDataAsset_Interface *data) {
}

void IDBAdapterBase::Stop() {
}
