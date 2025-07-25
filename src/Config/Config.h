#pragma once

#include "Module.h"

#include <yaml-cpp/node/node.h>


class BASE_API UConfig final : public IModuleBase {
    DECLARE_MODULE(UConfig)

protected:
    UConfig();

    void Initial() override;

public:
    ~UConfig() override;

    [[nodiscard]] constexpr const char *GetModuleName() const override {
        return "Config";
    }

    [[nodiscard]] const YAML::Node &GetServerConfig() const;

protected:
    YAML::Node mServerConfig;
};
