#pragma once

#include "Common.h"

#include <memory>
#include <vector>


class UObject;

class BASE_API UGCTable final {

public:
    UGCTable();
    ~UGCTable();

    DISABLE_COPY_MOVE(UGCTable)

    void Collect();

private:
    void Mark(UObject *node);
    void Sweep();

private:
    std::vector<std::unique_ptr<UObject>> mAllNodes;
    std::vector<UObject *> mRoots;
};

