#include "ConfigManager.h"
#include "Config.h"

#include <spdlog/spdlog.h>
#include <ranges>


UConfigManager::UConfigManager() {
}

UConfigManager::~UConfigManager() {
    for (const auto &val : mConfigMap | std::views::values) {
        delete val;
    }
}

int UConfigManager::LoadConfig(const UConfig *module) {
    if (module == nullptr) {
        SPDLOG_ERROR("{} - UConfig Is Null", __FUNCTION__);
        return -1;
    }

    for (auto &[type, val] : mConfigMap) {
        if (val == nullptr) {
            SPDLOG_ERROR("{} - LogicConfig[{}] Is Null", __FUNCTION__, type.name());
            return -2;
        }

        std::map<std::string, const nlohmann::json *> result;
        for (const auto &path : val->InitialPathArray()) {
            if (const auto data = module->FindConfig(path); data != nullptr) {
                result[path] = data;
            }
        }

        if (const int ret = val->Initial(result); ret != 0) {
            SPDLOG_ERROR("{:<20} - LogicConfig[{}] Initialize Failed, Result = {}", __FUNCTION__, type.name(), ret);
            return ret;
        }
    }

    return 0;
}
