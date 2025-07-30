#pragma once

#include "Module.h"

#include <memory>


class UConnection;


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

    void OnAcceptClient(const std::shared_ptr<UConnection> &conn);
};


