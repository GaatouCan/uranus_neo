#pragma once

#include "PlayerAgent.h"
#include "ComponentModule.h"


class BASE_API UPlayerBase : public IPlayerAgent {


public:
    UPlayerBase();
    ~UPlayerBase() override;

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
