#include "AgentHandlerImpl.h"
#include "gateway/PlayerAgent.h"
#include "internal/Packet.h"
#include "Server.h"
#include "service/ServiceModule.h"
#include "Utils.h"

#include <login.pb.h>
#include <spdlog/details/os.h>


FPackageHandle UAgentHandlerImpl::OnLoginSuccess(const int64_t pid) {
    const auto pkg = GetAgent()->BuildPackage();
    auto pkt = pkg.CastTo<FPacket>();

    Login::LoginResponse response;

    response.set_player_id(pid);

    if (const auto *module = GetServer()->GetModule<UServiceModule>()) {
        for (const auto &[id, name] : module->GetAllServiceMap()) {
            auto *info = response.add_services();

            info->set_sid(id);
            info->set_name(name);
        }
    }

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
    const auto pkg = GetAgent()->BuildPackage();
    auto pkt = pkg.CastTo<FPacket>();

    Login::LoginRepeatedResponse response;

    response.set_player_id(GetAgent()->GetPlayerID());
    response.set_other_address(addr);
    response.set_timepoint(utils::UnixTime());

    pkt->SetData(response.SerializeAsString());

    return pkt;
}

FPlatformInfo UAgentHandlerImpl::ParsePlatformInfo(const FPackageHandle &pkg) {
    const auto pkt = pkg.CastTo<FPacket>();

    Login::PlatformInfo request;
    request.ParseFromString(pkt->ToString());

    FPlatformInfo info;

    info.playerID = request.player_id();
    info.operateSystemName = request.os_name();
    info.operateSystemVersion = request.os_version();
    info.clientVersion = request.client_version();

    return info;
}

FLogoutRequest UAgentHandlerImpl::ParseLogoutRequest(const FPackageHandle &pkg) {
    const auto pkt = pkg.CastTo<FPacket>();

    Login::LogoutRequest request;
    request.ParseFromString(pkt->ToString());

    FLogoutRequest info;
    info.player_id = request.player_id();

    return info;
}
