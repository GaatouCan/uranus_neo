#pragma once

#include "Module.h"
#include "base/Types.h"
#include "base/MultiIOContextPool.h"

#include <absl/container/flat_hash_map.h>
#include <shared_mutex>
#include <memory>


class UConnection;

using std::shared_ptr;
using std::make_shared;

class BASE_API UNetwork final : public IModuleBase {

    DECLARE_MODULE(UNetwork)

public:
    UNetwork();
    ~UNetwork() override;

    [[nodiscard]] constexpr const char *GetModuleName() const override {
        return "Network";
    }

    shared_ptr<UConnection> FindConnection(const std::string &key) const;

    void RemoveConnection(const std::string &key);

protected:
    void Start() override;
    void Stop() override;

private:
    awaitable<void> WaitForClient(uint16_t port);

private:
    io_context mIOContext;
    ATcpAcceptor mAcceptor;
    UMultiIOContextPool mPool;

    absl::flat_hash_map<std::string, shared_ptr<UConnection>> mConnMap;
    mutable std::shared_mutex mMutex;
};
