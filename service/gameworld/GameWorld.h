#pragma once

#include <ServiceBase.h>
#include <Config/ConfigManager.h>

#include <memory>
#include <typeindex>
#include <absl/container/flat_hash_map.h>


class UGameWorld final : public IServiceBase {


public:
    UGameWorld();
    ~UGameWorld() override;

    [[nodiscard]] std::string GetServiceName() const override {
        return "Game World";
    }

    bool Initial(const IDataAsset_Interface *data) override;
    bool Start() override;
    void Stop() override;

    void OnPackage(const std::shared_ptr<IPackage_Interface> &pkg) override;

private:

};

