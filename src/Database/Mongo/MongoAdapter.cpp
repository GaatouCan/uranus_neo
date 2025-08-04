#include "MongoAdapter.h"
#include "MongoAdapterStartUp.h"
#include "Database/DBTaskBase.h"

#include <spdlog/spdlog.h>

UMongoAdapter::UMongoAdapter() {
}

UMongoAdapter::~UMongoAdapter() {
}

void UMongoAdapter::Initial(const IDataAsset_Interface *data) {
    const auto *startUp = dynamic_cast<const FMongoAdapterStartUp *>(data);
    if (startUp == nullptr)
        return;

    mPool = std::make_unique<mongocxx::pool>(mongocxx::uri(startUp->mUri));

    mWorkerList = std::vector<FWorkerNode>(2);
    for (auto &worker: mWorkerList) {
        worker.thread = std::thread([this, &worker] {
            const auto client = mPool->acquire();
            auto db = client["demo"];

            while (worker.deque.IsRunning()) {
                worker.deque.Wait();

                if (!worker.deque.IsRunning())
                    break;

                try {
                    const auto node = std::move(worker.deque.PopFront());
                    node->Execute(*client, db);
                } catch (const std::exception &e) {
                    SPDLOG_ERROR("UDataAccess::RunInThread - Exception: {}", e.what());
                }
            }

            worker.deque.Clear();
        });
    }
}
