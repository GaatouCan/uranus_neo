#pragma once

#include "DatabaseTask.h"


class BASE_API UDatabase_Query final : public IDatabaseTask_Interface {

public:
    ~UDatabase_Query() override = default;

    void Execute(mongocxx::database &db) override;
};

