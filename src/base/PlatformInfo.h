#pragma once

#include "Common.h"

#include <string>

struct BASE_API FPlatformInfo {
    int64_t playerID;
    std::string operateSystemName;
    std::string operateSystemVersion;
    std::string operateSystemBuild;
    std::string clientVersion;
    std::string clientBuild;
    std::string CPUName;
    std::string CPUBrand;
    int CPUCores;
    std::string memorySize;
};