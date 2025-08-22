#pragma once

#include "Module.h"

#include <memory>
#include <unordered_map>
#include <string>


class UAgent;
class IPluginBase;


class BASE_API UMonitor final : public IModuleBase {

    DECLARE_MODULE(UMonitor)

protected:
    void Initial() override;
    void Start() override;
    void Stop() override;

public:
    UMonitor();
    ~UMonitor() override;

    [[nodiscard]] constexpr const char *GetModuleName() const override {
        return "UMonitor";
    }

    // void OnAcceptClient(const std::shared_ptr<UAgent> &conn);

private:
    std::unordered_map<std::string, std::unique_ptr<IPluginBase>> mPluginMap;
};


