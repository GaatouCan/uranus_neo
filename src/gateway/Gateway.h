#pragma once

#include "Module.h"
#include "base/Types.h"
#include "base/MultiIOContextPool.h"

#include <absl/container/flat_hash_map.h>
#include <shared_mutex>
#include <memory>


class UAgent;
class IPlayerBase;

using std::shared_ptr;
using std::make_shared;

class BASE_API UGateway final : public IModuleBase {

    DECLARE_MODULE(UGateway)

public:
    UGateway();
    ~UGateway() override;

    [[nodiscard]] constexpr const char *GetModuleName() const override {
        return "Network";
    }

    shared_ptr<UAgent> FindAgent(const std::string &key) const;
    void RemoveAgent(const std::string &key);

protected:
    void Start() override;
    void Stop() override;

private:
    awaitable<void> WaitForClient(uint16_t port);

private:
    io_context mIOContext;
    ATcpAcceptor mAcceptor;
    UMultiIOContextPool mPool;

    absl::flat_hash_map<std::string, shared_ptr<UAgent>> mConnMap;
    mutable std::shared_mutex mConnMutex;

    absl::flat_hash_map<int64_t, shared_ptr<UAgent>> mPlayerMap;
    mutable std::shared_mutex mPlayerMutex;
};
