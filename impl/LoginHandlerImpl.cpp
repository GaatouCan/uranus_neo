#include "LoginHandlerImpl.h"
#include "Server.h"
#include "internal/Packet.h"
#include "base/Recycler.h"
#include "service/ServiceModule.h"

#include <login.pb.h>


ULoginHandlerImpl::ULoginHandlerImpl(ULoginAuth *module)
    : ILoginHandler(module) {
}

ULoginHandlerImpl::~ULoginHandlerImpl() {
}

void ULoginHandlerImpl::UpdateAddressList() {
}

ILoginHandler::FLoginToken ULoginHandlerImpl::ParseLoginRequest(const FPackageHandle &pkg) {
    const auto pkt = pkg.CastTo<FPacket>();
    if (pkt == nullptr)
        return {};

    if (pkt->GetPackageID() != LOGIN_REQUEST_PACKAGE_ID)
        return {};

    Login::LoginRequest request;
    request.ParseFromString(pkt->ToString());

    return {
        request.token(),
        request.player_id()
    };
}

FPlatformInfo ULoginHandlerImpl::ParsePlatformInfo(const FPackageHandle &pkg) {
    const auto pkt = pkg.CastTo<FPacket>();
    if (pkt == nullptr)
        return {};

    if (pkt->GetPackageID() != PLATFORM_PACKAGE_ID)
        return {};

    Login::PlatformInfo request;
    request.ParseFromString(pkt->ToString());

    FPlatformInfo info;

    info.operateSystemName = request.os_name();
    info.operateSystemVersion = request.os_version();
    info.clientVersion = request.client_version();

    return info;
}

void ULoginHandlerImpl::OnAgentError(const int64_t pid, const std::string &addr, const FPackageHandle &pkg, const std::string &desc) {
    const auto pkt = pkg.CastTo<FPacket>();
    if (pkt == nullptr)
        return;

    Login::LoginFailedResponse response;

    response.set_player_id(pid);
    response.set_address(addr);
    response.set_reason(Login::LoginFailedResponse::AGENT_ERROR);
    response.set_extend(desc);

    pkt->SetPackageID(LOGIN_FAILED_PACKAGE_ID);
    pkt->SetData(response.SerializeAsString());
}

void ULoginHandlerImpl::OnLoginSuccess(const int64_t pid, const FPackageHandle &pkg) const {
    const auto *service = GetServer()->GetModule<UServiceModule>();
    if (service == nullptr)
        return;

    const auto pkt = pkg.CastTo<FPacket>();
    if (pkt == nullptr)
        return;

    Login::LoginResponse res;

    res.set_player_id(pid);

    for (const auto &[name, id]: service->GetServiceList()) {
        auto *val = res.add_services();
        val->set_name(name);
        val->set_sid(id);
    }

    pkt->SetPackageID(LOGIN_RESPONSE_PACKAGE_ID);
    pkt->SetData(res.SerializeAsString());
}

void ULoginHandlerImpl::OnRepeatLogin(const int64_t pid, const std::string &addr, const FPackageHandle &pkg) {
    const auto pkt = pkg.CastTo<FPacket>();
    if (pkt == nullptr)
        return;

    Login::LoginFailedResponse response;
    response.set_player_id(pid);
    response.set_address(addr);
    response.set_reason(Login::LoginFailedResponse::REPEATED);

    pkt->SetPackageID(LOGIN_FAILED_PACKAGE_ID);
    pkt->SetData(response.SerializeAsString());
}
