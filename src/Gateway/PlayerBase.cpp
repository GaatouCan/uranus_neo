#include "PlayerBase.h"

UPlayerBase::UPlayerBase() {
}

UPlayerBase::~UPlayerBase() {
}

void UPlayerBase::OnPackage(const std::shared_ptr<FPackage> &pkg) {
    mRoute.OnReceivePackage(pkg, this);
}
