#pragma once

#include "Module.h"
#include "base/SharedLibrary.h"

#include <functional>
#include <shared_mutex>
#include <unordered_map>


class IPackage_Interface;
class IServiceBase;
class IPlayerAgent;
class UAgentContext;

using std::unordered_map;


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

    void OnPlayerLogin(int64_t pid, const std::string &key);
    void OnPlayerLogout(int64_t pid);

    std::string GetConnectionKey(int64_t pid) const;
    std::shared_ptr<UAgentContext> FindPlayerAgent(int64_t pid) const;

    void SendToPlayer(int64_t pid, const std::shared_ptr<IPackage_Interface> &pkg) const;
    void PostToPlayer(int64_t pid, const std::function<void(IServiceBase *)> &task) const;

    void OnClientPackage(int64_t pid, const std::shared_ptr<IPackage_Interface> &pkg) const;
    void SendToClient(int64_t pid, const std::shared_ptr<IPackage_Interface> &pkg) const;

    void OnHeartBeat(int64_t pid, const std::shared_ptr<IPackage_Interface> &pkg) const;

private:
    FSharedLibrary mLibrary;

    unordered_map<std::string, int64_t> mConnToPlayer;
    unordered_map<int64_t, std::shared_ptr<UAgentContext>> mPlayerMap;

    mutable std::shared_mutex mMutex;
};
