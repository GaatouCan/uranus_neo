#include "PluginBase.h"
#include "Monitor.h"


IPluginBase::IPluginBase()
    : mOwner(nullptr) {
}

IPluginBase::~IPluginBase() {
}

UMonitor *IPluginBase::GetMonitor() const {
    return mOwner;
}

UServer *IPluginBase::GetServer() const {
    return mOwner->GetServer();
}

void IPluginBase::OnAcceptClient(const std::shared_ptr<UConnection> &conn) {
}

void IPluginBase::SetUpModule(UMonitor *owner) {
    mOwner = owner;
}
