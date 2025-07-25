#pragma once

#include "Module.h"

#include <yaml-cpp/yaml.h>


constexpr auto SERVER_CONFIG_FILE = "/server.yaml";
constexpr auto SERVER_CONFIG_JSON = "/json";


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

    void SetYAMLPath(const std::string &path);
    void SetJSONPath(const std::string &path);

    [[nodiscard]] const YAML::Node &GetServerConfig() const;

protected:
    std::string mYAMLPath;
    std::string mJSONPath;

    YAML::Node mServerConfig;
};
