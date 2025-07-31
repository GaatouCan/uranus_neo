#pragma once

#include "PlayerAgent.h"
#include "Base/ProtocolRoute.h"
#include "ComponentModule.h"

#include <functional>


class BASE_API UPlayerBase : public IPlayerAgent {

    using APlayerProtocol = std::function<void(uint32_t, const std::shared_ptr<FPacket> &, UPlayerBase *)>;

public:
    UPlayerBase();
    ~UPlayerBase() override;

    void RegisterProtocol(uint32_t id, const APlayerProtocol &func);

    void OnPackage(const std::shared_ptr<FPacket> &pkg) override;

    UComponentModule &GetComponentModule();

    template<CComponentType Type>
    Type *CreateComponent();

    template<CComponentType Type>
    Type *GetComponent() const;

    virtual void OnLogin();
    virtual void OnLogout();

protected:
    bool Start() override;
    void Stop() override;

private:
    /** Protocol Route While Receive Package **/
    TProtocolRoute<APlayerProtocol> mRoute;

    /** Manage All The Player Components **/
    UComponentModule mComponentModule;
};

template<CComponentType Type>
inline Type *UPlayerBase::CreateComponent() {
    return mComponentModule.CreateComponent<Type>();
}

template<CComponentType Type>
inline Type *UPlayerBase::GetComponent() const {
    return mComponentModule.GetComponent<Type>();
}
