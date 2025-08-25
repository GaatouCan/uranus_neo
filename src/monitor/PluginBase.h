#pragma once

#include "Common.h"

#include <memory>

class UMonitor;
class UServer;
class UPlayerAgent;

class BASE_API IPluginBase {

    friend class UMonitor;

public:
    IPluginBase();
    virtual ~IPluginBase();

    DISABLE_COPY_MOVE(IPluginBase)

    [[nodiscard]] virtual const char *GetPluginName() const = 0;

    [[nodiscard]] UMonitor *GetMonitor() const;
    [[nodiscard]] UServer *GetServer() const;

    // virtual void OnAcceptClient(const std::shared_ptr<UAgent> &conn);

private:
    void SetUpModule(UMonitor *owner);

private:
    UMonitor *mOwner;
};

