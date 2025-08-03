#pragma once

#include "DBTaskBase.h"


class BASE_API UDBTask_Update final : public TDBTaskBase {

public:
    UDBTask_Update();
    ~UDBTask_Update() override;

    void Execute(mongocxx::database &db) override;
};
