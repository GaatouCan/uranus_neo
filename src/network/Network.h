#pragma once

#include "Module.h"
#include "base/Package.h"
#include "base/CodecFactory.h"
#include "base/RecycleHandle.h"

#include <thread>
#include <shared_mutex>


class UConnection;

using FPackageHandle = FRecycleHandle<IPackage_Interface>;
using std::shared_ptr;

class BASE_API UNetwork final : public IModuleBase {

    DECLARE_MODULE(UNetwork)

    /** The Separate IO Context For Handling Network IO **/
    io_context mIOContext;

    /** Client Acceptor **/
    ATcpAcceptor mAcceptor;

    /** The Separate Thread For Handling Network Event **/
    std::thread mThread;

    /** Used To Generate Codec For Every Connection **/
    unique_ptr<ICodecFactory_Interface> mCodecFactory;

    /** The Package Pool For Network Module **/
    unique_ptr<IRecyclerBase> mPackagePool;

    std::unordered_map<std::string, shared_ptr<UConnection>> mConnectionMap;
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

    template<class Type, class... Args>
    requires std::derived_from<Type, ICodecFactory_Interface>
    void SetCodecFactory(Args && ... args);

    [[nodiscard]] io_context &GetIOContext();

    [[nodiscard]] unique_ptr<IRecyclerBase> CreatePackagePool() const;
    FPackageHandle BuildPackage() const;

    shared_ptr<UConnection> FindConnection(const std::string &key) const;
    void RemoveConnection(const std::string &key, int64_t pid);

    void SendToClient(const std::string &key, const FPackageHandle &pkg) const;

    void OnLoginSuccess(const std::string &key, int64_t pid, const FPackageHandle &pkg) const;
    void OnLoginFailure(const std::string &key, const FPackageHandle &pkg) const;

private:
    awaitable<void> WaitForClient(uint16_t port);
};

template<class Type, class ... Args> requires std::derived_from<Type, ICodecFactory_Interface>
inline void UNetwork::SetCodecFactory(Args &&...args) {
    if (mState != EModuleState::CREATED)
        return;

    mCodecFactory = make_unique<Type>(std::forward<Args>(args)...);
}
