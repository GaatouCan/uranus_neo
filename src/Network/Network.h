#pragma once

#include "Module.h"
#include "Base/Types.h"
#include "Base/Recycler.h"

#include <memory>
#include <string>
#include <vector>
#include <thread>


class FPackage;
class UConnection;


class BASE_API UNetwork final : public IModuleBase {

    DECLARE_MODULE(UNetwork)

    io_context mIOContext;
    ATcpAcceptor mAcceptor;

    std::thread mThread;

    shared_ptr<IRecyclerBase> mPackagePool;

    std::unordered_map<int64_t, shared_ptr<UConnection>> mConnectionMap;
    mutable std::shared_mutex mMutex;

protected:
    UNetwork();

    void Initial() override;
    void Start() override;
    void Stop() override;

public:
    ~UNetwork() override;

    [[nodiscard]] constexpr const char *GetModuleName() const override {
        return "Network";
    }

    [[nodiscard]] io_context &GetIOContext();

    const unsigned char* GetEncryptionKey();

    std::shared_ptr<FPackage> BuildPackage();

private:
    awaitable<void> WaitForClient(uint16_t port);

    static std::vector<uint8_t> HexToBytes(const std::string &hex);
};

