#pragma once

#include "Common.h"

#include <mongocxx/database.hpp>


class BASE_API IDatabaseTask_Interface {

public:
    IDatabaseTask_Interface() = default;
    virtual ~IDatabaseTask_Interface() = default;

    DISABLE_COPY_MOVE(IDatabaseTask_Interface)

    virtual void Execute(mongocxx::database &db) = 0;
};
