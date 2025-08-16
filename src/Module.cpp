#include "Module.h"

#include <format>
#include <stdexcept>

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
    if (mServer == nullptr)
        throw std::runtime_error(std::format("{} - Server Is NULL Pointer", __FUNCTION__));
    return mServer;
}

EModuleState IModuleBase::GetState() const {
    return mState;
}
