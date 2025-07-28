#pragma once

#include "Module.h"
#include "Utils.h"

#include <shared_mutex>
#include <absl/container/flat_hash_map.h>


class IPackageInterface;
class IServiceBase;
class IPlayerAgent;
class UAgentContext;
class FLibraryHandle;

using absl::flat_hash_map;


class BASE_API UGateway final : public IModuleBase {

    DECLARE_MODULE(UGateway)

protected:
    UGateway();

    void Initial() override;
    void Stop() override;

public:
    ~UGateway() override;

    constexpr const char *GetModuleName() const override {
        return "Gateway Module";
    }

    void OnPlayerLogin(int64_t pid, int64_t cid);
    void OnPlayerLogout(int64_t pid);

    int64_t GetConnectionID(int64_t pid) const;
    std::shared_ptr<UAgentContext> FindPlayerAgent(int64_t pid) const;

    void SendToPlayer(int64_t pid, const std::shared_ptr<IPackageInterface> &pkg) const;
    void PostToPlayer(int64_t pid, const std::function<void(IServiceBase *)> &task) const;

    void OnClientPackage(int64_t pid, const std::shared_ptr<IPackageInterface> &pkg) const;
    void SendToClient(int64_t pid, const std::shared_ptr<IPackageInterface> &pkg) const;

    void OnHeartBeat(int64_t pid, const std::shared_ptr<IPackageInterface> &pkg) const;

private:
    FLibraryHandle *mLibrary;

    flat_hash_map<int64_t, int64_t> mConnToPlayer;
    flat_hash_map<int64_t, std::shared_ptr<UAgentContext>> mPlayerMap;

    mutable std::shared_mutex mMutex;
};
