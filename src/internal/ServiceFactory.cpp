#include "ServiceFactory.h"
#include "service/ServiceBase.h"
#include "Utils.h"

#include <filesystem>
#include <spdlog/spdlog.h>


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
            FSharedLibrary library(entry.path());

            if (!library.IsValid()) {
                SPDLOG_ERROR("{} - Load Core Library[{}] Failed", __FUNCTION__, entry.path().string());
                continue;
            }

            const auto creator = library.GetSymbol<AServiceCreator>("CreateInstance");
            const auto destroyer = library.GetSymbol<AServiceDestroyer>("DestroyInstance");

            if (creator == nullptr || destroyer == nullptr) {
                SPDLOG_ERROR("{} - Load Core Library[{}] Symbol Failed", __FUNCTION__, entry.path().string());
                continue;
            }

            mCoreMap.insert_or_assign(filename, { library, creator, destroyer });
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
            FSharedLibrary library(entry.path());

            if (!library.IsValid()) {
                SPDLOG_ERROR("{} - Load Extend Library[{}] Failed", __FUNCTION__, entry.path().string());
                continue;
            }

            const auto creator = library.GetSymbol<AServiceCreator>("CreateInstance");
            const auto destroyer = library.GetSymbol<AServiceDestroyer>("DestroyInstance");

            if (creator == nullptr || destroyer == nullptr) {
                SPDLOG_ERROR("{} - Load Extend Library[{}] Symbol Failed", __FUNCTION__, entry.path().string());
                continue;
            }

            mExtendMap.insert_or_assign(filename, { library, creator, destroyer });
        }
    }
}

FServiceHandle UServiceFactory::CreateInstance(const std::string &path) {
    const std::vector<std::string> res = utils::SplitString(path, '.');

    if (res.size() != 2)
        return {};

    if (res[0] == "core") {
        if (const auto iter = mCoreMap.find(res[1]); iter != mCoreMap.end()) {
            if (const auto creator = iter->second.creator; creator != nullptr) {
                auto *pService = std::invoke(creator);
                return {pService, this, path};
            }
        }
    }

    if (res[0] == "extend") {
        if (const auto iter = mExtendMap.find(res[1]); iter != mExtendMap.end()) {
            if (const auto creator = iter->second.creator; creator != nullptr) {
                auto *pService = std::invoke(creator);
                return {pService, this, path};
            }
        }
    }

    return {};
}

void UServiceFactory::DestroyInstance(IServiceBase *pService, const std::string &path) {
    const std::vector<std::string> res = utils::SplitString(path, '.');

    if (res.size() != 2) {
        SPDLOG_WARN("{} - Parse Path[{}] Failed, Use Default Delete", __FUNCTION__, path);
        delete pService;
        return;
    }

    if (res[0] == "core") {
        if (const auto iter = mCoreMap.find(res[1]); iter != mCoreMap.end()) {
            if (const auto destroyer = iter->second.destroyer; destroyer != nullptr) {
                std::invoke(destroyer, pService);
                return;
            }
        }
    }

    if (res[0] == "extend") {
        const auto iter = mExtendMap.find(res[1]);
        if (iter != mExtendMap.end()) {
            if (const auto destroyer = iter->second.destroyer; destroyer != nullptr) {
                std::invoke(destroyer, pService);
                return;
            }
        }
    }

    SPDLOG_WARN("{} - Failed To Find Path[{}], Use Default Delete", __FUNCTION__, path);
    delete pService;
}
