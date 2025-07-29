#include "PlayerBase.h"

UPlayerBase::UPlayerBase() {
}

UPlayerBase::~UPlayerBase() {
}

void UPlayerBase::RegisterProtocol(const uint32_t id, const APlayerProtocol &func) {
    mRoute.Register(id, func);
}

void UPlayerBase::OnPackage(const std::shared_ptr<FPackage> &pkg) {
    mRoute.OnReceivePackage(pkg, this);
}
