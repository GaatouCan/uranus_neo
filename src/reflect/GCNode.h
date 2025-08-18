#pragma once

#include "Common.h"

#include <unordered_set>

class UObject;
class UGCTable;

class BASE_API IGCNodeBase {

    friend class UGCTable;

public:
    IGCNodeBase();
    virtual ~IGCNodeBase() = default;

    DISABLE_COPY_MOVE(IGCNodeBase)

private:
    std::unordered_set<IGCNodeBase *> mFields;
    bool bMarked;
};