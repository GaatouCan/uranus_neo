#include "Module.h"

IModuleBase::IModuleBase()
    : mServer(nullptr),
      mState(EModuleState::CREATED) {
}

void IModuleBase::SetUpModule(UServer *server) {
    mServer = server;
}

void IModuleBase::Initial() {
    mState = EModuleState::INITIALIZED;
}

void IModuleBase::Start() {
    mState = EModuleState::RUNNING;
}

void IModuleBase::Stop() {
    mState = EModuleState::STOPPED;
}

UServer *IModuleBase::GetServer() const {
    return mServer;
}

EModuleState IModuleBase::GetState() const {
    return mState;
}
