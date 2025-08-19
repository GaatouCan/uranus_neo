#include "GcTable.h"
#include "Object.h"

UGCTable::UGCTable() {
}

UGCTable::~UGCTable() {
}

void UGCTable::Mark(UObject *node) {
    if (node == nullptr)
        return;

    if (node->mControl.bMarked)
        return;

    node->mControl.bMarked= true;

    for (const auto &val : node->mControl.nodes) {
        this->Mark(val);
    }
}

void UGCTable::Sweep() {
    std::erase_if(mAllNodes,[](const std::unique_ptr<UObject> &node) {
        return !node->mControl.bMarked;
    });

    for (const auto &val : mAllNodes) {
        val->mControl.bMarked = false;
    }
}

void UGCTable::Collect() {
    for (const auto &val : mRoots) {
        this->Mark(val);
    }

    this->Sweep();
}
