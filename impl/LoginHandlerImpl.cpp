#include "LoginHandlerImpl.h"
#include "Server.h"
#include "Internal/Packet.h"
#include "Service/ServiceModule.h"

#include <login.pb.h>


ULoginHandlerImpl::ULoginHandlerImpl(ULoginAuth *module)
    : ILoginHandler(module) {
}

ULoginHandlerImpl::~ULoginHandlerImpl() {
}

void ULoginHandlerImpl::UpdateAddressList() {
}

ILoginHandler::FLoginToken ULoginHandlerImpl::ParseLoginRequest(const std::shared_ptr<IPackage_Interface> &pkg) {
    const auto pkt = std::dynamic_pointer_cast<FPacket>(pkg);
    if (pkt == nullptr)
        return {};

    if (pkt->GetPackageID() != 1002)
        return {};

    Login::LoginRequest request;
    request.ParseFromString(pkt->ToString());

    return {
        request.token(),
        request.player_id()
    };
}

void ULoginHandlerImpl::OnLoginSuccess(int64_t pid, const std::shared_ptr<IPackage_Interface> &pkg) const {
    const auto *service = GetServer()->GetModule<UServiceModule>();
    if (service == nullptr)
        return;

    const auto pkt = std::dynamic_pointer_cast<FPacket>(pkg);
    if (pkt == nullptr)
        return;

    Login::LoginResponse res;

    res.set_player_id(pid);

    for (const auto &[name, id]: service->GetServiceList()) {
        auto *val = res.add_services();
        val->set_name(name);
        val->set_sid(id);
    }

    pkt->SetPackageID(1003);
    pkt->SetData(res.SerializeAsString());
}

void ULoginHandlerImpl::OnRepeatLogin(int64_t pid, const std::string &addr, const std::shared_ptr<IPackage_Interface> &pkg) {
}
