#include "LoggerModule.h"
#include "Server.h"
#include "config/Config.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/daily_file_sink.h>

ULoggerModule::ULoggerModule() {
}

ULoggerModule::~ULoggerModule() {
}

void ULoggerModule::Stop() {
    Super::Stop();
    for (const auto &name : mUseCount | std::views::keys) {
        spdlog::drop(name);
    }
}

void ULoggerModule::TryCreateLogger(const std::string &name) {
    if (mState >= EModuleState::STOPPED)
        return;

    if (!mUseCount.contains(name)) {
        const auto *config = GetServer()->GetModule<UConfig>();
        if (!config)
            return;

        const auto &cfg = config->GetServerConfig();

        const auto rootDir = cfg["server"]["logger"]["directory"].as<std::string>();
        const auto loggerPath = rootDir + "/" + name;

        auto logger = spdlog::daily_logger_mt(name, loggerPath, 2, 0);
    }

    std::unique_lock lock(mMutex);
    mUseCount[name]++;
}

void ULoggerModule::TryDestroyLogger(const std::string &name) {
    {
        std::unique_lock lock(mMutex);
        const auto iter = mUseCount.find(name);
        if (iter == mUseCount.end())
            return;

        --iter->second;

        if (iter->second > 0)
            return;

        mUseCount.erase(iter);
    }

    spdlog::drop(name);
}

int ULoggerModule::GetLoggerUseCount(const std::string &name) const {
    std::shared_lock lock(mMutex);
    const auto iter = mUseCount.find(name);
    if (iter == mUseCount.end())
        return 0;
    return iter->second;
}
