#pragma once

#include "Database/DBAdapter.h"


class BASE_API UMongoAdapter final : public IDBAdapter_Interface {

public:
    UMongoAdapter();
    ~UMongoAdapter() override;

    void Initial(const IDataAsset_Interface *data) override;
};
