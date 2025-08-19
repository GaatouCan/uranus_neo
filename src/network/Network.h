#pragma once

#include "base/Package.h"
#include "base/CodecFactory.h"
#include "base/RecycleHandle.h"

#include <absl/container/flat_hash_map.h>
#include <shared_mutex>


class UConnection;
class UServer;
using FPackageHandle = FRecycleHandle<IPackage_Interface>;

class BASE_API UNetwork final {

    friend class UServer;

private:
    UNetwork();

    void SetUpModule(UServer *server);

public:
    ~UNetwork();

    DISABLE_COPY_MOVE(UNetwork)

    void OnAccept(ATcpSocket &&socket);
    void RemoveConnection(const std::string &key);

    template<typename T, typename... Args>
    requires std::derived_from<T, ICodecFactory_Interface>
    void SetCodecFactory(Args && ... args);

private:
    UServer *mServer;

    absl::flat_hash_map<std::string, shared_ptr<UConnection>> mConnMap;
    mutable std::shared_mutex mMutex;

    unique_ptr<ICodecFactory_Interface> mCodecFactory;
};

template<typename T, typename ... Args>
requires std::derived_from<T, ICodecFactory_Interface>
void UNetwork::SetCodecFactory(Args &&...args) {
    mCodecFactory = make_unique<T>(std::forward<Args>(args)...);
}
