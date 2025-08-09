#pragma once

#include "ComponentModule.h"

#include <Gateway/PlayerAgent.h>
#include <Base/ProtocolRoute.h>
#include <Config/ConfigManager.h>
#include <Internal/Packet.h>

#include <functional>


class UPlayer final : public IPlayerAgent {

    using AProtocolFunctor = std::function<void(uint32_t, const std::shared_ptr<FPacket>&, UPlayer *)>;

public:
    UPlayer();
    ~UPlayer() override;

    void RegisterProtocol(uint32_t id, const AProtocolFunctor &func);

    void OnPackage(const std::shared_ptr<IPackage_Interface> &pkg) override;

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
    RegisterProtocol(static_cast<uint32_t>(protocol::EProtoType::##type), &protocol::##func);
