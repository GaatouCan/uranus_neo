#include "GcTable.h"
#include "ObjectNode.h"

UGCTable::UGCTable() {
}

UGCTable::~UGCTable() {
}

void UGCTable::Mark(IObjectNodeBase *node) {
    if (node == nullptr)
        return;

    if (node->bMarked)
        return;

    node->bMarked = true;

    for (const auto &val : node->mFields) {
        this->Mark(val);
    }
}

void UGCTable::Sweep() {
    std::erase_if(mAllNodes,[](const auto &node) {
        return !node->bMarked;
    });

    for (const auto &val : mAllNodes) {
        val->bMarked = false;
    }
}

void UGCTable::Collect() {
    for (const auto &val : mRoots) {
        this->Mark(val);
    }

    this->Sweep();
}
