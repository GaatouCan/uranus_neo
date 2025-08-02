#pragma once

#include "Common.h"

#include <vector>
#include <nlohmann/json.hpp>

class BASE_API ILogicConfig_Interface {

public:
    ILogicConfig_Interface() = default;
    virtual ~ILogicConfig_Interface() = default;

    DISABLE_COPY_MOVE(ILogicConfig_Interface)

    [[nodiscard]] virtual std::vector<std::string> InitialPathArray() const = 0;

    virtual bool Initial(const std::map<std::string, const nlohmann::json *> &config) = 0;
};
