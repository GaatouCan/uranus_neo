#pragma once

#include "DatabaseTask.h"


template<class Callback>
class  UDatabase_Query final : public IDatabaseTask_Interface {

    Callback mCallback;

public:
    ~UDatabase_Query() override = default;

    void Execute(mongocxx::database &db) override {

    }
};

