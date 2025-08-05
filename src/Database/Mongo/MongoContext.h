#pragma once

#include "Database/DBContext.h"

#include <mongocxx/pool.hpp>

struct BASE_API FMongoContext final : public IDBContext_Interface {
    mongocxx::pool::entry mEntry;

    explicit FMongoContext(mongocxx::pool::entry entry)
        : mEntry(std::move(entry)) {
    }
};
