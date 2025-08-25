#pragma once

#include "ComponentModule.h"
#include "gateway/PlayerBase.h"

#include <base/ProtocolRoute.h>
#include <config/ConfigManager.h>
#include <internal/Packet.h>

#include <functional>

class UPlayer final : public IPlayerBase {

    using AProtocolFunctor = std::function<void(uint32_t, FPacket *, UPlayer *)>;

public:
    UPlayer();
    ~UPlayer() override;

    void Initial() override;

    void OnLogin() override;
    void OnLogout() override;

    void Save() override;

    void OnReset() override;

    void OnPackage(IPackage_Interface *pkg) override;
    void OnEvent(IEventParam_Interface *event) override;

    void RegisterProtocol(uint32_t id, const AProtocolFunctor &func);

    UComponentModule &GetComponentModule();

    template<CComponentType Type>
    Type *CreateComponent();

    template<CComponentType Type>
    Type *GetComponent() const;

    template<class T>
    requires std::derived_from<T, ILogicConfig_Interface>
    T *GetLogicConfig() const;

private:
    void LoadProtocol();

private:
    UComponentModule mComponentModule;
    TProtocolRoute<FPacket, AProtocolFunctor> mRoute;
    UConfigManager mConfig;
};

template<CComponentType Type>
inline Type *UPlayer::CreateComponent() {
    return mComponentModule.CreateComponent<Type>();
}

template<CComponentType Type>
inline Type *UPlayer::GetComponent() const {
    return mComponentModule.GetComponent<Type>();
}

template<class T> requires std::derived_from<T, ILogicConfig_Interface>
inline T *UPlayer::GetLogicConfig() const {
    return mConfig.GetLogicConfig<T>();
}


#define REGISTER_PROTOCOL(type, func) \
    RegisterProtocol(static_cast<uint32_t>(protocol::EProtoType::type), &protocol::##func);
