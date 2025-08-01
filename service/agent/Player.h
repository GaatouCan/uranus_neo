#pragma once

#include <Gateway/PlayerBase.h>
#include <Base/ProtocolRoute.h>
#include <Config/ConfigManager.h>
#include <Internal/Packet.h>

#include <functional>


class UPlayer final : public UPlayerBase {

    using AProtocolFunctor = std::function<void(uint32_t, const std::shared_ptr<FPacket>&, UPlayer *)>;

public:
    UPlayer();
    ~UPlayer() override;

    void RegisterProtocol(uint32_t id, const AProtocolFunctor &func);

    void OnPackage(const std::shared_ptr<IPackage_Interface> &pkg) override;

    template<class T>
    requires std::derived_from<T, ILogicConfig_Interface>
    T *GetLogicConfig() const;

private:
    void LoadProtocol();

private:
    TProtocolRoute<FPacket, AProtocolFunctor> mRoute;

    UConfigManager mConfig;
};

template<class T> requires std::derived_from<T, ILogicConfig_Interface>
inline T *UPlayer::GetLogicConfig() const {
    return mConfig.GetLogicConfig<T>();
}


#define REGISTER_PROTOCOL(type, func) \
    RegisterProtocol(static_cast<uint32_t>(protocol::EProtoType::##type), &protocol::##func);
