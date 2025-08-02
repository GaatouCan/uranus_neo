#pragma once

#include "DatabaseTask.h"


class BASE_API UDBTask_Update final : public IDatabaseTask_Interface {

public:
    UDBTask_Update();
    ~UDBTask_Update() override;

    void Execute(mongocxx::database &db) override;
};
