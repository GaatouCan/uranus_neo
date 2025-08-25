#pragma once

#include "Module.h"

#include <unordered_map>
#include <shared_mutex>


class BASE_API ULoggerModule final : public IModuleBase {

    DECLARE_MODULE(ULoggerModule)

protected:
    void Stop() override;

public:
    ULoggerModule();
    ~ULoggerModule() override;

    [[nodiscard]] constexpr const char * GetModuleName() const override {
        return "Logger Module";
    }

    void TryCreateLogger(const std::string &name);
    void TryDestroyLogger(const std::string &name);

    [[nodiscard]] int GetLoggerUseCount(const std::string &name) const;

private:
    std::unordered_map<std::string, int> mUseCount;
    mutable std::shared_mutex mMutex;
};

