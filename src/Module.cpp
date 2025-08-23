#include "Module.h"
#include "Server.h"

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
    if (mState != EModuleState::CREATED)
        throw std::logic_error(std::format("{} - Module[{}] Not In CREATED State", __FUNCTION__, GetModuleName()));

    mState = EModuleState::INITIALIZED;
}

void IModuleBase::Start() {
    if (mState != EModuleState::INITIALIZED)
        throw std::logic_error(std::format("{} - Module[{}] Not In INITIALIZED State", __FUNCTION__, GetModuleName()));

    mState = EModuleState::RUNNING;
}

void IModuleBase::Stop() {
    mState = EModuleState::STOPPED;
}

UServer *IModuleBase::GetServer() const {
    if (mServer == nullptr)
        throw std::logic_error(std::format("{} - Server Is NULL Pointer", __FUNCTION__));
    return mServer;
}

asio::io_context &IModuleBase::GetIOContext() const {
    if (mServer == nullptr)
        throw std::logic_error(std::format("{} - Server Is NULL Pointer", __FUNCTION__));

    return mServer->GetIOContext();
}

EModuleState IModuleBase::GetState() const {
    return mState;
}
