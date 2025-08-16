#pragma once

#include "Module.h"
#include "base/CodecFactory.h"
#include "base/Recycler.h"

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

    /** Used To Generate Codec For Every Connection **/
    ICodecFactory_Interface *mCodecFactory;

    /** The Package Pool For Network Module **/
    IRecyclerBase *mPackagePool;

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

    [[nodiscard]] IRecyclerBase *CreatePackagePool() const;
    std::shared_ptr<IPackage_Interface> BuildPackage() const;

    shared_ptr<UConnection> FindConnection(const std::string &key) const;
    void RemoveConnection(const std::string &key, int64_t pid);

    void SendToClient(const std::string &key, const shared_ptr<IPackage_Interface> &pkg) const;

    void OnLoginSuccess(const std::string &key, int64_t pid, const shared_ptr<IPackage_Interface> &pkg) const;
    void OnLoginFailure(const std::string &key, const shared_ptr<IPackage_Interface> &pkg) const;

private:
    awaitable<void> WaitForClient(uint16_t port);
};

template<class Type, class ... Args> requires std::derived_from<Type, ICodecFactory_Interface>
inline void UNetwork::SetCodecFactory(Args &&...args) {
    if (mState != EModuleState::CREATED)
        return;

    mCodecFactory = new Type(std::forward<Args>(args)...);
}
