#pragma once

#include "LogicConfig.h"

#include <typeindex>
#include <unordered_map>

class UConfig;


class BASE_API UConfigManager final {

public:
    UConfigManager();
    ~UConfigManager();

    DISABLE_COPY_MOVE(UConfigManager)

    int LoadConfig(const UConfig* module);

    template<class T>
    requires std::derived_from<T, ILogicConfig_Interface>
    void CreateLogicConfig();

    template<class T>
    requires std::derived_from<T, ILogicConfig_Interface>
    T *GetLogicConfig() const;

private:
    std::unordered_map<std::type_index, ILogicConfig_Interface *> mConfigMap;
};

template<class T> requires std::derived_from<T, ILogicConfig_Interface>
inline void UConfigManager::CreateLogicConfig() {
    if (mConfigMap.contains(typeid(T)))
        return;

    mConfigMap.insert(std::make_pair(std::type_index(typeid(T)), new T));
}

template<class T> requires std::derived_from<T, ILogicConfig_Interface>
inline T *UConfigManager::GetLogicConfig() const {
    const auto iter = mConfigMap.find(typeid(T));
    return iter != mConfigMap.end() ? dynamic_cast<T *>(iter->second) : nullptr;
}
