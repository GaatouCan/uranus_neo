#include "Monitor.h"
#include "PluginBase.h"

#include <ranges>

UMonitor::UMonitor() {
}

void UMonitor::Initial() {
    if (mState != EModuleState::CREATED)
        return;

    mState = EModuleState::INITIALIZED;
}

void UMonitor::Start() {
    if (mState != EModuleState::INITIALIZED)
        return;

    mState = EModuleState::RUNNING;
}

void UMonitor::Stop() {
    if (mState == EModuleState::STOPPED)
        return;

    mState = EModuleState::STOPPED;
}

UMonitor::~UMonitor() {
    Stop();
}

// void UMonitor::OnAcceptClient(const std::shared_ptr<UConnection> &conn) {
//     for (const auto &val : mPluginMap | std::views::values) {
//         val->OnAcceptClient(conn);
//     }
// }
