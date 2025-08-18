#pragma once

#include "Common.h"

#include <memory>
#include <vector>


class IObjectNodeBase;

class BASE_API UGCTable final {

public:
    UGCTable();
    ~UGCTable();

    DISABLE_COPY_MOVE(UGCTable)

    void Collect();

private:
    void Mark(IObjectNodeBase *node);
    void Sweep();

private:
    std::vector<std::unique_ptr<IObjectNodeBase>> mAllNodes;
    std::vector<IObjectNodeBase *> mRoots;
};

