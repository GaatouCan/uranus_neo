#pragma once

#include "PlayerComponent.h"

#include <unordered_map>
#include <typeindex>
#include <memory>

#include <bsoncxx/builder/basic/document.hpp>


class UPlayerBase;

class BASE_API UComponentModule final {

public:
    UComponentModule() = delete;
    explicit UComponentModule(UPlayerBase *player);

    DISABLE_COPY_MOVE(UComponentModule)

    ~UComponentModule();

    [[nodiscard]] UPlayerBase *GetPlayer() const;
    [[nodiscard]] int64_t GetPlayerID() const;

    template<CComponentType Type>
    Type *CreateComponent();

    template<CComponentType Type>
    Type *GetComponent() const;

    void OnLogin();
    void OnLogout();

    void Serialize(bsoncxx::builder::basic::document &builder);
    void Deserialize(const bsoncxx::document::view &document);

private:
    UPlayerBase *mOwner;

    std::unordered_map<std::type_index, std::unique_ptr<IPlayerComponent>> mComponents;
    std::vector<std::type_index> mComponentOrder;
};

template<CComponentType Type>
inline Type *UComponentModule::CreateComponent() {
    const auto type = std::type_index(typeid(Type));

    if (const auto it = mComponents.find(type); it != mComponents.end()) {
        return it->second;
    }

    auto component = std::make_unique<Type>();
    component->SetUpModule(this);

    auto *ptr = component.get();

    mComponents.insert_or_assign(type, std::move(component));
    mComponentOrder.push_back(type);

    return ptr;
}

template<CComponentType Type>
inline Type *UComponentModule::GetComponent() const {
    const auto iter = mComponents.find(typeid(Type));
    return iter != mComponents.end() ? iter->second.get() : nullptr;
}
