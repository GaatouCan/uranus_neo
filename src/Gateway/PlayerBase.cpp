#include "PlayerBase.h"
#include "Database/DataAccess.h"

UPlayerBase::UPlayerBase()
    : mComponentModule(this) {
}

UPlayerBase::~UPlayerBase() {
}

UComponentModule &UPlayerBase::GetComponentModule() {
    return mComponentModule;
}

asio::awaitable<bool> UPlayerBase::AsyncInitial(const IDataAsset_Interface *data) {
    auto *dataAccess = GetModule<UDataAccess>();
    if (!dataAccess)
        co_return false;

    const auto filter = bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("player_id", GetPlayerID()));
    mongocxx::options::find opt;
    const auto res = co_await dataAccess->AsyncFindOne("player", filter, opt, asio::use_awaitable);
    if (!res.has_value())
        co_return false;

    const auto doc = res.value();
    if (const auto elem = doc["components"]; elem && elem.type() == bsoncxx::type::k_document) {
        mComponentModule.Deserialize(elem.get_document().view());
    }

    co_return true;
}

void UPlayerBase::OnLogin() {
    mComponentModule.OnLogin();
}

void UPlayerBase::OnLogout() {
    mComponentModule.OnLogout();
}

bool UPlayerBase::Start() {
    IPlayerAgent::Start();
    this->OnLogin();
    return true;
}

void UPlayerBase::Stop() {
    IPlayerAgent::Stop();
    this->OnLogout();
}
