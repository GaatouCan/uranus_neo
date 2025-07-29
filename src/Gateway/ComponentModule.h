#pragma once

#include "PlayerComponent.h"

#include <unordered_map>
#include <typeindex>


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

private:
    UPlayerBase *mOwner;

    std::unordered_map<std::type_index, IPlayerComponent *> mComponents;
    std::vector<std::type_index> mComponentOrder;
};

template<CComponentType Type>
inline Type *UComponentModule::CreateComponent() {
    const auto type = std::type_index(typeid(Type));

    if (const auto it = mComponents.find(type); it != mComponents.end()) {
        return it->second;
    }

    auto *component = new Type();
    component->SetUpModule(this);

    mComponents[type] = component;
    mComponentOrder.push_back(type);

    return component;
}

template<CComponentType Type>
inline Type *UComponentModule::GetComponent() const {
    const auto iter = mComponents.find(typeid(Type));
    return iter != mComponents.end() ? iter->second : nullptr;
}
