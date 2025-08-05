#pragma once

#include "Database/DBContextBase.h"

#include <mongocxx/pool.hpp>

struct BASE_API FMongoContext final : public IDBContextBase {
    mongocxx::pool::entry mEntry;

    explicit FMongoContext(mongocxx::pool::entry entry)
        : mEntry(std::move(entry)) {
    }
};
