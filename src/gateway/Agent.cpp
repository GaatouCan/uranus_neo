#include "Agent.h"
#include "Gateway.h"
#include "network/Connection.h"

#include <format>

UAgent::UAgent()
    : mGateway(nullptr),
      mPlayerID(-1) {
}

UAgent::~UAgent() {
}

int64_t UAgent::GetPlayerID() const {
    // TODO
    return 0;
}

UGateway *UAgent::GetGateway() const {
    return mGateway;
}

UServer *UAgent::GetUServer() const {
    if (!mGateway)
        throw std::logic_error(std::format("{} - Gateway Is Null Pointer", __FUNCTION__));

    return mGateway->GetServer();
}

void UAgent::SetUpModule(UGateway *gateway) {
    mGateway = gateway;
}

void UAgent::SetUpConnection(const shared_ptr<UConnection> &conn) {
    mConn = conn;
}

void UAgent::SetUpPlayerID(const int64_t pid) {
    mPlayerID = pid;
}
