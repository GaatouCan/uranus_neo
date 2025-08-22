#pragma once

#include "Module.h"

#include <yaml-cpp/yaml.h>
#include <nlohmann/json.hpp>
#include <unordered_map>


inline constexpr auto SERVER_CONFIG_FILE = "/server.yaml";
inline constexpr auto SERVER_CONFIG_JSON = "/json";


class BASE_API UConfig final : public IModuleBase {
    DECLARE_MODULE(UConfig)

protected:
    void Initial() override;

public:
    UConfig();
    ~UConfig() override;

    [[nodiscard]] constexpr const char *GetModuleName() const override {
        return "Config";
    }

    void SetYAMLPath(const std::string &path);
    void SetJSONPath(const std::string &path);

    [[nodiscard]] const YAML::Node &GetServerConfig() const;

    [[nodiscard]] const nlohmann::json *FindConfig(const std::string &path) const;

protected:
    std::string mYAMLPath;
    std::string mJSONPath;

    YAML::Node mServerConfig;
    std::unordered_map<std::string, nlohmann::json> mConfigMap;
};
