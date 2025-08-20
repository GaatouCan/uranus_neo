#pragma once

#include "Module.h"

#include <memory>
#include <absl/container/flat_hash_map.h>
#include <shared_mutex>


class UAgent;
using std::shared_ptr;


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

    [[nodiscard]] bool IsAgentExist(int64_t pid) const;

    [[nodiscard]] shared_ptr<UAgent> FindAgent(int64_t pid) const;
    void RemoveAgent(int64_t pid);

    void OnPlayerLogin(const std::string &key, int64_t pid);

private:
    shared_ptr<UAgent> CreateAgent(int64_t pid);

private:
    absl::flat_hash_map<int64_t, shared_ptr<UAgent>> mAgentMap;
    mutable std::shared_mutex mMutex;
};
