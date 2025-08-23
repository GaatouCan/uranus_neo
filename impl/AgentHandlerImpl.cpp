#include "AgentHandlerImpl.h"
#include "../src/gateway/PlayerAgent.h"
#include "internal/Packet.h"

#include <login.pb.h>
#include <spdlog/details/os.h>

FPackageHandle UAgentHandlerImpl::OnLoginSuccess(int64_t pid) {
    const auto pkg = GetAgent()->BuildPackage();
    auto pkt = pkg.CastTo<FPacket>();

    Login::LoginResponse response;

    response.set_player_id(pid);

    pkt->SetData(response.SerializeAsString());
    return pkt;
}

FPackageHandle UAgentHandlerImpl::OnLoginFailure(int code, const std::string &desc) {
    const auto pkg = GetAgent()->BuildPackage();
    auto pkt = pkg.CastTo<FPacket>();

    Login::LoginFailedResponse response;

    response.set_player_id(GetAgent()->GetPlayerID());
    response.set_reason(static_cast<Login::LoginFailedResponse::Reason>(code));
    response.set_description(desc);

    pkt->SetData(response.SerializeAsString());
    return pkt;
}

FPackageHandle UAgentHandlerImpl::OnRepeated(const std::string &addr) {
    // TODO
    return {};
}

FPlatformInfo UAgentHandlerImpl::ParsePlatformInfo(const FPackageHandle &pkg) {
    // TODO
    return {};
}
