#pragma once

#include <../../src/service/ServiceBase.h>

#include <entt/entt.hpp>
#include <absl/container/flat_hash_map.h>


class UGameWorld final : public IServiceBase {


public:
    UGameWorld();
    ~UGameWorld() override;

    [[nodiscard]] std::string GetServiceName() const override {
        return "Game World";
    }

    awaitable<bool> AsyncInitial(const IDataAsset_Interface *data) override;
    bool Start() override;
    void Stop() override;

    void OnPackage(IPackage_Interface *pkg) override;
    void OnEvent(IEventParam_Interface *event) override;
    void OnUpdate(ASteadyTimePoint now, ASteadyDuration delta) override;

private:
    entt::registry mRegistry;
};

