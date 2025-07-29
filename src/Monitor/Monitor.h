#pragma once

#include "Module.h"


class BASE_API UMonitor final : public IModuleBase {

    DECLARE_MODULE(UMonitor)

protected:
    UMonitor();

    void Initial() override;
    void Start() override;
    void Stop() override;

public:
    ~UMonitor() override;

    [[nodiscard]] constexpr const char *GetModuleName() const override {
        return "UMonitor";
    }
};


