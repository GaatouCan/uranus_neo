#pragma once

#include "Database/DBContext.h"

#include <mongocxx/pool.hpp>

struct BASE_API FMongoContext final : public IDBContext_Interface {
    mongocxx::pool::entry entry;

    explicit FMongoContext(mongocxx::pool::entry e)
        : entry(std::move(e)) {
    }
};
