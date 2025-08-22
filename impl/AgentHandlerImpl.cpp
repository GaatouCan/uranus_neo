#include "AgentHandlerImpl.h"
#include "Agent.h"
#include "internal/Packet.h"

#include <login.pb.h>

FPackageHandle UAgentHandlerImpl::OnLoginSuccess(int64_t pid) {

    const auto pkg = GetAgent()->BuildPackage();
    auto pkt = pkg.CastTo<FPacket>();

    Login::LoginResponse response;

    response.set_player_id(pid);

    pkt->SetData(response.SerializeAsString());
    return pkt;
}

FPackageHandle UAgentHandlerImpl::OnLoginFailure(int code, const std::string &desc) {
}

FPackageHandle UAgentHandlerImpl::OnRepeated(const std::string &addr) {
}

FPlatformInfo UAgentHandlerImpl::ParsePlatformInfo(const FPackageHandle &pkg) {
}
