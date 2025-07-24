#include "Network.h"

void UNetwork::Initial() {
    if (mState != EModuleState::CREATED)
        return;

    mState = EModuleState::INITIALIZED;
}

std::shared_ptr<FPackage> UNetwork::BuildPackage() {
    return nullptr;
}
