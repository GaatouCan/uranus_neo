#include "Recycle.h"

bool IRecycleInterface::CopyFrom(IRecycleInterface *other) {
    if (other != nullptr && other != this) {
        return true;
    }
    return false;
}

bool IRecycleInterface::CopyFrom(const std::shared_ptr<IRecycleInterface> &other) {
    if (other != nullptr && other.get() != this) {
        return true;
    }
    return false;
}
