#pragma once

#include "Common.h"

#include <memory>
#include <vector>


class IGCNodeBase;

class BASE_API UGCTable final {

public:
    UGCTable();
    ~UGCTable();

    DISABLE_COPY_MOVE(UGCTable)

    void Collect();

private:
    void Mark(IGCNodeBase *node);
    void Sweep();

private:
    std::vector<std::unique_ptr<IGCNodeBase>> mAllNodes;
    std::vector<IGCNodeBase *> mRoots;
};

