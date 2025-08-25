#pragma once

#include "Module.h"
#include "base/Recycler.h"

#include <map>


class IPackage_Interface;
class IActorBase;

using FPackageHandle = FRecycleHandle<IPackage_Interface>;
using AActorTask = std::function<void(IActorBase *)>;


class BASE_API URouteModule final : public IModuleBase {

    DECLARE_MODULE(URouteModule)

public:
    URouteModule();
    ~URouteModule() override;

    [[nodiscard]] constexpr  const char *GetModuleName() const override {
        return "Route Module";
    }

    /// If Target Equal PLAYER_TARGET_ID, Then Use pid
    void PostPackage(const FPackageHandle &pkg, int64_t pid = -1) const;

    /// Only Post To Service
    void PostPackage(const std::string &name, const FPackageHandle &pkg) const;

    /// If ToService Is True, Target Is Service ID, Else Target Is Player ID
    void PostTask(bool bToService, int64_t target, const AActorTask &task) const;

    /// Only Post To Service
    void PostTask(const std::string &name, const AActorTask &task) const;

    /// Send Package To Client
    void SendToClient(int64_t pid, const FPackageHandle &pkg) const;

    [[nodiscard]] std::map<int64_t, std::string> GetRouteMap() const;
};
