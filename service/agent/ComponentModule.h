#pragma once

#include "PlayerComponent.h"

#include <unordered_map>
#include <typeindex>
#include <memory>


class UPlayer;
class UServer;

class UComponentModule final {

public:
    UComponentModule();
    ~UComponentModule();

    DISABLE_COPY_MOVE(UComponentModule)

    void SetUpPlayer(UPlayer* player);
    [[nodiscard]] UPlayer* GetPlayer() const;
    [[nodiscard]] int64_t GetPlayerID() const;

    [[nodiscard]] UServer* GetServer() const;

    template<CComponentType Type>
    Type *CreateComponent();

    template<CComponentType Type>
    Type *GetComponent() const;

    void OnLogin();
    void OnLogout();

private:
    UPlayer *mPlayer;

    std::unordered_map<std::type_index, std::unique_ptr<IPlayerComponent>> mComponentMap;
};

template<CComponentType Type>
inline Type *UComponentModule::CreateComponent() {
    if (mPlayer == nullptr)
        return nullptr;

    if (const auto iter = mComponentMap.find(std::type_index(typeid(Type))); iter != mComponentMap.end()) {
        return dynamic_cast<Type *>(iter->second.get());
    }

    auto *comp = new Type();
    comp->SetUpModule(this);

    mComponentMap.insert_or_assign(typeid(Type), comp);
    return comp;
}

template<CComponentType Type>
inline Type *UComponentModule::GetComponent() const {
    if (const auto iter = mComponentMap.find(std::type_index(typeid(Type))); iter != mComponentMap.end()) {
        return dynamic_cast<Type *>(iter->second.get());
    }
    return nullptr;
}
