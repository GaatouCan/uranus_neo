#pragma once

#include "Module.h"
#include "base/CodecFactory.h"
#include "base/Recycler.h"
#include "base/IdentAllocator.h"

#include <memory>
#include <thread>
#include <atomic>


class IPackage_Interface;
class UConnection;


class BASE_API UNetwork final : public IModuleBase {

    DECLARE_MODULE(UNetwork)

    struct BASE_API FNetworkNode {
        std::thread th;
        io_context ctx;
    };

    io_context mIOContext;
    ATcpAcceptor mAcceptor;

    std::vector<FNetworkNode> mWorkerList;
    std::atomic_size_t mNextIndex;

    unique_ptr<ICodecFactory_Interface> mCodecFactory;
    TIdentAllocator<int64_t, true> mIDAllocator;

    std::unordered_map<int64_t, shared_ptr<UConnection>> mConnectionMap;
    mutable std::shared_mutex mConnectionMutex;

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

    template<class Type, class... Args>
    requires std::derived_from<Type, ICodecFactory_Interface>
    void SetCodecFactory(Args && ... args);

    [[nodiscard]] io_context &GetIOContext();

    [[nodiscard]] shared_ptr<IRecyclerBase> CreatePackagePool() const;
    // std::shared_ptr<IPackage_Interface> BuildPackage() const;

    shared_ptr<UConnection> FindConnection(int64_t cid) const;
    void RemoveConnection(int64_t cid, int64_t pid);

    void SendToClient(int64_t cid, const shared_ptr<IPackage_Interface> &pkg) const;

    void OnLoginSuccess(int64_t cid, int64_t pid, const shared_ptr<IPackage_Interface> &pkg) const;
    void OnLoginFailure(int64_t cid, const shared_ptr<IPackage_Interface> &pkg) const;

    void RecycleID(int64_t cid);
private:
    io_context &GetNextIOContext();
    awaitable<void> WaitForClient(uint16_t port);
};

template<class Type, class ... Args> requires std::derived_from<Type, ICodecFactory_Interface>
inline void UNetwork::SetCodecFactory(Args &&...args) {
    if (mState != EModuleState::CREATED)
        return;

    mCodecFactory = std::make_unique<Type>(std::forward<Args>(args)...);
}
