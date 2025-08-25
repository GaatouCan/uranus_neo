#include "ServiceFactory.h"
#include "Utils.h"

#include <filesystem>

inline constexpr auto LINUX_LIBRARY_PREFIX = "lib";

void UServiceFactory::LoadService() {
    for (const auto &entry: std::filesystem::directory_iterator(CORE_SERVICE_DIRECTORY)) {
#if defined(_WIN32) || defined(_WIN64)
        if (entry.is_regular_file() && entry.path().extension() == ".dll") {
            const auto filename = entry.path().stem().string();
#elif
        if (entry.is_regular_file() && entry.path().extension() == ".so") {
            auto filename = entry.path().stem().string();
            if (filename.compare(0, strlen(LINUX_LIBRARY_PREFIX), LINUX_LIBRARY_PREFIX) == 0) {
                filename.erase(0, strlen(LINUX_LIBRARY_PREFIX));
            }
#endif
            mCoreMap[filename] = FSharedLibrary{entry.path()};
        }
    }

    for (const auto &entry: std::filesystem::directory_iterator(EXTEND_SERVICE_DIRECTORY)) {
#if defined(_WIN32) || defined(_WIN64)
        if (entry.is_regular_file() && entry.path().extension() == ".dll") {
            const auto filename = entry.path().stem().string();
#elif
        if (entry.is_regular_file() && entry.path().extension() == ".so") {
            auto filename = entry.path().stem().string();
            if (filename.compare(0, strlen(LINUX_LIBRARY_PREFIX), LINUX_LIBRARY_PREFIX) == 0) {
                filename.erase(0, strlen(LINUX_LIBRARY_PREFIX));
            }
#endif
            mExtendMap[filename] = FSharedLibrary{entry.path()};
        }
    }
}

FServiceHandle UServiceFactory::CreateInstance(const std::string &name) const {
    const std::vector<std::string> res = utils::SplitString(name, '.');

    if (res.size() != 2)
        return {};

    FSharedLibrary library;

    if (res[0] == "core") {
        const auto iter = mCoreMap.find(res[1]);
        if (iter == mCoreMap.end())
            return {};

        library = iter->second;
    }

    if (res[0] == "extend") {
        const auto iter = mExtendMap.find(res[1]);
        if (iter == mExtendMap.end())
            return {};

        library = iter->second;
    }

    if (!library.IsValid())
        return {};

    return FServiceHandle{ library };
}
