#include "MongoAdapter.h"
#include "MongoStartUpData.h"
#include "MongoContext.h"

#include <spdlog/spdlog.h>

UMongoAdapter::UMongoAdapter()
    : bQuit(false) {
}

UMongoAdapter::~UMongoAdapter() {
}

void UMongoAdapter::Initial(const IDataAsset_Interface *data) {
    const auto *startUp = dynamic_cast<const FMongoStartUpData *>(data);
    if (startUp == nullptr)
        return;

    mPool = std::make_unique<mongocxx::pool>(mongocxx::uri(startUp->mUri));
    mDatabaseName = startUp->mDatabaseName;
}

void UMongoAdapter::Stop() {
    bQuit = true;
}

IDBContext_Interface *UMongoAdapter::AcquireContext() {
    const auto res = new FMongoContext(mPool->acquire());
    return res;
}
