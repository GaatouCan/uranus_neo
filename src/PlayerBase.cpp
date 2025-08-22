#include "PlayerBase.h"

IPlayerBase::IPlayerBase() {
}

IPlayerBase::~IPlayerBase() {
}

int64_t IPlayerBase::GetPlayerID() const {
    // TODO:
    return -1;
}

void IPlayerBase::Initial() {
}

void IPlayerBase::OnLogin() {
}

void IPlayerBase::OnLogout() {
}

void IPlayerBase::Save() {
    // TODO
}

void IPlayerBase::OnPackage(IPackage_Interface *pkg) {

}

void IPlayerBase::OnEvent(IEventParam_Interface *event) {
}

void IPlayerBase::OnReset() {

}
