#include "Recycle.h"

bool IRecycle_Interface::CopyFrom(IRecycle_Interface *other) {
    if (other != nullptr && other != this) {
        return true;
    }
    return false;
}

bool IRecycle_Interface::CopyFrom(const std::shared_ptr<IRecycle_Interface> &other) {
    if (other != nullptr && other.get() != this) {
        return true;
    }
    return false;
}
