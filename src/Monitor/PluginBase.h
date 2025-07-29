#pragma once

#include "Common.h"


class UMonitor;
class UServer;

class BASE_API IPluginBase {

    friend class UMonitor;

public:
    IPluginBase();
    virtual ~IPluginBase();

    DISABLE_COPY_MOVE(IPluginBase)

    [[nodiscard]] virtual const char *GetPluginName() const = 0;

    [[nodiscard]] UMonitor *GetMonitor() const;
    [[nodiscard]] UServer *GetServer() const;

private:
    void SetUpModule(UMonitor *owner);

private:
    UMonitor *mOwner;
};

