#pragma once

#include "Common.h"

#include <unordered_set>

class UObject;
class UGCTable;

class BASE_API IObjectNodeBase {

    friend class UGCTable;

public:
    IObjectNodeBase();
    virtual ~IObjectNodeBase() = default;

    DISABLE_COPY_MOVE(IObjectNodeBase)

private:
    std::unordered_set<IObjectNodeBase *> mFields;
    bool bMarked;
};