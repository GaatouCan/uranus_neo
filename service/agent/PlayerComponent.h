#pragma once

#include "Common.h"

#include <cstdint>
#include <concepts>


class UComponentModule;
class UPlayer;
class UServer;

class IPlayerComponent {

    friend UComponentModule;

public:
    IPlayerComponent();
    virtual ~IPlayerComponent();

    DISABLE_COPY_MOVE(IPlayerComponent)

    [[nodiscard]] virtual const char *GetComponentName() const = 0;
    [[nodiscard]] virtual int GetComponentVersion() const = 0;

    [[nodiscard]] UPlayer *GetPlayer() const;
    [[nodiscard]] int64_t GetPlayerID() const;

    [[nodiscard]] UServer *GetServer() const;

    virtual void OnLogin();
    virtual void OnLogout();

    virtual void OnDayChange();

protected:
    void SetUpModule(UComponentModule *module);

private:
    UComponentModule* mModule;
};

template<class T>
concept CComponentType = std::derived_from<T, IPlayerComponent>;


#define DECLARE_COMPONENT(component) \
private: \
    friend class UServer; \
    using Super = IPlayerComponent; \
    using ThisClass = component;\
public: \
    DISABLE_COPY_MOVE(component) \
private: