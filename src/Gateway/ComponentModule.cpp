#include "ComponentModule.h"
#include "PlayerBase.h"

#include <ranges>

UComponentModule::UComponentModule(UPlayerBase *player)
    : mOwner(player) {
}

UComponentModule::~UComponentModule() {
}

UPlayerBase *UComponentModule::GetPlayer() const {
    return mOwner;
}

int64_t UComponentModule::GetPlayerID() const {
    return mOwner->GetPlayerID();
}

void UComponentModule::OnLogin() {
    for (const auto &type : mComponentOrder) {
        if (const auto it = mComponents.find(type); it != mComponents.end()) {
            it->second->OnLogin();
        }
    }
}

void UComponentModule::OnLogout() {
    for (const auto &type : mComponentOrder) {
        if (const auto it = mComponents.find(type); it != mComponents.end()) {
            it->second->OnLogout();
        }
    }
}

void UComponentModule::Serialize(bsoncxx::builder::basic::document &builder) {
    bsoncxx::builder::basic::document doc;
    for (const auto &component: mComponents | std::views::values) {
        component->Serialize(doc);
    }
    builder.append(bsoncxx::builder::basic::kvp("components", doc.extract()));
}

void UComponentModule::Deserialize(const bsoncxx::document::value &document) {
    for (const auto &component: mComponents | std::views::values) {
        const auto name = component->GetNameAndVersion().first;
        if (auto elem = document[name]; elem && elem.type() == bsoncxx::type::k_document) {
            component->Deserialize(elem.get_document().view());
        }
    }
}
