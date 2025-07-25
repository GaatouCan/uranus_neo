#include "ServiceModule.h"
#include "Config/Config.h"
#include "Server.h"

#include <spdlog/spdlog.h>
#include <filesystem>
#include <ranges>

#include "ServiceContext.h"

UServiceModule::UServiceModule() {
}

void UServiceModule::Initial() {
    if (mState != EModuleState::CREATED)
        return;

    const auto *configModule = GetServer()->GetModule<UConfig>();
    if (configModule == nullptr) {
        SPDLOG_ERROR("Config Module Not Found, Service Module Initialization Failed!");
        GetServer()->Shutdown();
        exit(-6);
    }

    const auto &config = configModule->GetServerConfig();

#ifdef __linux__
    const std::string prefix = "lib";
#endif

    SPDLOG_INFO("Loading Core Service...");

    if (config["service"] && config["service"]["core"]) {
        for (const auto &val : config["service"]["core"]) {
            const auto filename = val["name"].as<std::string>();
#if defined(_WIN32) || defined(_WIN64)
            const std::filesystem::path path = std::filesystem::path(CORE_SERVICE_DIRECTORY) / (filename + ".dll");
#else
            std::string linux_filename = filename;
            if (filename.compare(0, prefix.size(), prefix) != 0) {
                linux_filename = prefix + filename;
            }
            const std::filesystem::path path = std::filesystem::path(CORE_SERVICE_DIRECTORY) / (linux_filename + ".so");
#endif
            mCoreLibraryMap[filename] = FSharedLibrary(path);
        }
    } else {
        for (const auto &entry: std::filesystem::directory_iterator(CORE_SERVICE_DIRECTORY)) {
#if defined(_WIN32) || defined(_WIN64)
            if (entry.is_regular_file() && entry.path().extension() == ".dll") {
                const auto filename = entry.path().stem().string();
#else
            if (entry.is_regular_file() && entry.path().extension() == ".so") {
                auto filename = entry.path().stem().string();
                if (filename.compare(0, prefix.size(), prefix) == 0) {
                    filename.erase(0, prefix.size());
                }
#endif
                mCoreLibraryMap[filename] = FSharedLibrary(entry.path());
            }
        }
    }

    SPDLOG_INFO("Loading Extend Service...");

    if (config["service"] && config["service"]["extend"]) {
        for (const auto &val : config["service"]["extend"]) {
            const auto filename = val["name"].as<std::string>();
#if defined(_WIN32) || defined(_WIN64)
            const std::filesystem::path path = std::filesystem::path(EXTEND_SERVICE_DIRECTORY) / (filename + ".dll");
#else
            std::string linux_filename = filename;
            if (filename.compare(0, prefix.size(), prefix) != 0) {
                linux_filename = prefix + filename;
            }
            const std::filesystem::path path = std::filesystem::path(EXTEND_SERVICE_DIRECTORY) / (linux_filename + ".so");
#endif
           mExtendLibraryMap[filename] = FSharedLibrary(path);
        }
    } else {
        for (const auto &entry: std::filesystem::directory_iterator(EXTEND_SERVICE_DIRECTORY)) {
#if defined(_WIN32) || defined(_WIN64)
            if (entry.is_regular_file() && entry.path().extension() == ".dll") {
                const auto filename = entry.path().stem().string();
#else
            if (entry.is_regular_file() && entry.path().extension() == ".so") {
                auto filename = entry.path().stem().string();
                if (filename.compare(0, prefix.size(), prefix) == 0) {
                    filename.erase(0, prefix.size());
                }

#endif
                mExtendLibraryMap[filename] = FSharedLibrary(entry.path());
            }
        }
    }


    // Begin Initial Core Service

    for (const auto &[filename, node] : mCoreLibraryMap) {
        const int32_t sid = mAllocator.Allocate();
        const auto context = std::make_shared<UServiceContext>();

        context->SetUpModule(this);
        context->SetUpLibrary(node);
        context->SetServiceID(sid);
        context->SetFilename(filename);
        context->SetCoreFlag(true);

        if (context->Initial(nullptr)) {
            const auto name = context->GetServiceName();

            mServiceMap[sid] = context;
            mFilenameMapping[filename].insert(sid);
            mNameToServiceID[name] = sid;

            SPDLOG_INFO("Initialized Service[{}]", name);
        } else {
            SPDLOG_ERROR("Failed To Initialize Service[{}]", filename);
            GetServer()->Shutdown();
            exit(-8);
        }
    }

    mState = EModuleState::INITIALIZED;
}

void UServiceModule::Start() {
    if (mState != EModuleState::INITIALIZED)
        return;

    for (const auto &context: mServiceMap | std::views::values) {
        if (context->BootService()) {
            SPDLOG_INFO("Starting Core Service[{}]...", context->GetServiceName());
        } else {
            SPDLOG_ERROR("Failed To Start Core Service[{}]", context->GetServiceName());
            GetServer()->Shutdown();
            exit(-9);
        }
    }

    mState = EModuleState::RUNNING;
}

void UServiceModule::Stop() {

}

UServiceModule::~UServiceModule() {
    Stop();
}
