#pragma once

#include "Common.h"

#include <cstdint>
#include <concepts>


class UComponentModule;
class UPlayerBase;


class BASE_API IPlayerComponent {


public:
    IPlayerComponent();
    virtual ~IPlayerComponent();

    DISABLE_COPY_MOVE(IPlayerComponent)

    void SetUpModule(UComponentModule* module);

    [[nodiscard]] UPlayerBase* GetPlayer() const;
    [[nodiscard]] int64_t GetPlayerID() const;

    virtual void OnLogin();
    virtual void OnLogout();

private:
    UComponentModule *mModule;
};

template<class T>
concept CComponentType = std::derived_from<T, IPlayerComponent>;
