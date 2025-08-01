#include "Config.h"
#include "Utils.h"

#include <cassert>
#include <fstream>
#include <spdlog/spdlog.h>


UConfig::UConfig() {
}

void UConfig::Initial() {
    if (mState != EModuleState::CREATED)
        return;

    SPDLOG_INFO("Using Server Configuration File: {}.", mYAMLPath + SERVER_CONFIG_FILE);

    mServerConfig = YAML::LoadFile(mYAMLPath + SERVER_CONFIG_FILE);

    assert(!mServerConfig.IsNull());
    SPDLOG_INFO("Checking Server Configuration File.");

    assert(!mServerConfig["server"].IsNull());
    assert(!mServerConfig["server"]["id"].IsNull());
    assert(!mServerConfig["server"]["port"].IsNull());
    assert(!mServerConfig["server"]["worker"].IsNull());
    assert(!mServerConfig["server"]["cross"].IsNull());

    assert(!mServerConfig["server"]["encryption"].IsNull());
    assert(!mServerConfig["server"]["encryption"]["key"].IsNull());

    assert(!mServerConfig["package"].IsNull());
    assert(!mServerConfig["package"]["magic"].IsNull());

    assert(!mServerConfig["service"].IsNull());

    SPDLOG_INFO("Server Configuration File Check Successfully.");

    const std::string jsonPath = !mJSONPath.empty() ? mJSONPath : mYAMLPath + SERVER_CONFIG_JSON;

    utils::TraverseFolder(jsonPath, [this, jsonPath](const std::filesystem::directory_entry &entry) {
        if (entry.path().extension().string() == ".json") {
            std::ifstream fs(entry.path());

            auto filepath = entry.path().string();
            filepath = filepath.substr(
                strlen(jsonPath.c_str()) + 1,
                filepath.length() - 6 - strlen(jsonPath.c_str()));

#ifdef WIN32
            filepath = utils::StringReplace(filepath, '\\', '.');
#elifdef __linux__
                filepath = utils::StringReplace(filepath, '/', '.');
#else
                filepath = utils::StringReplace(filepath, '/', '.');
#endif

            mConfigMap[filepath] = nlohmann::json::parse(fs);
            SPDLOG_INFO("\tLoaded {}.", filepath);
        }
    });

    SPDLOG_INFO("JSON Files Loaded Successfully.");
    mState = EModuleState::INITIALIZED;
}

UConfig::~UConfig() {
}

void UConfig::SetYAMLPath(const std::string &path) {
    mYAMLPath = path;
}

void UConfig::SetJSONPath(const std::string &path) {
    mJSONPath = path;
}

const YAML::Node &UConfig::GetServerConfig() const {
    return mServerConfig;
}

const nlohmann::json *UConfig::FindConfig(const std::string &path) const {
    const auto iter = mConfigMap.find(path);
    return iter != mConfigMap.end() ? &iter->second : nullptr;
}
