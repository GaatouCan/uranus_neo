#include "PlayerBase.h"

IPlayerBase::IPlayerBase() {
}

IPlayerBase::~IPlayerBase() {
}

int64_t IPlayerBase::GetPlayerID() const {
    // TODO:
    return -1;
}

void IPlayerBase::Save() {
    // TODO
}

void IPlayerBase::OnPackage(IPackage_Interface *pkg) const {

}

void IPlayerBase::OnRepeat() {

}
