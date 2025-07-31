#pragma once

#include "Module.h"
#include "Base/CodecFactory.h"
#include "Base/Recycler.h"

#include <memory>
#include <thread>


class IPackage_Interface;
class UConnection;


class BASE_API UNetwork final : public IModuleBase {

    DECLARE_MODULE(UNetwork)

    /** The Separate IO Context For Handling Network IO **/
    io_context mIOContext;

    /** Client Acceptor **/
    ATcpAcceptor mAcceptor;

    /** The Separate Thread For Handling Network Event **/
    std::thread mThread;

    /** The Package Pool For Network Module **/
    shared_ptr<IRecyclerBase> mPackagePool;

    /** Used To Generate Codec For Every Connection **/
    unique_ptr<ICodecFactory_Interface> mCodecFactory;

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

    template<class Type, class... Args>
    requires std::derived_from<Type, ICodecFactory_Interface>
    void SetCodecFactory(Args && ... args);

    [[nodiscard]] io_context &GetIOContext();

    [[nodiscard]] shared_ptr<IRecyclerBase> CreatePackagePool() const;
    std::shared_ptr<IPackage_Interface> BuildPackage() const;

    shared_ptr<UConnection> FindConnection(int64_t cid) const;
    void RemoveConnection(int64_t cid, int64_t pid);

    void SendToClient(int64_t cid, const shared_ptr<IPackage_Interface> &pkg) const;

    void OnLoginSuccess(int64_t cid, int64_t pid, const shared_ptr<IPackage_Interface> &pkg) const;
    void OnLoginFailure(int64_t cid, const shared_ptr<IPackage_Interface> &pkg) const;

private:
    awaitable<void> WaitForClient(uint16_t port);
};

template<class Type, class ... Args> requires std::derived_from<Type, ICodecFactory_Interface>
inline void UNetwork::SetCodecFactory(Args &&...args) {
    if (mState != EModuleState::CREATED)
        return;

    mCodecFactory = std::make_unique<Type>(std::forward<Args>(args)...);
}
