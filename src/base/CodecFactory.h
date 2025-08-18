#pragma once

#include "Common.h"
#include "Types.h"

#include <memory>


class IRecyclerBase;
class IPackageCodec_Interface;

using std::unique_ptr;
using std::make_unique;


class BASE_API ICodecFactory_Interface {

public:
    ICodecFactory_Interface() = default;
    virtual ~ICodecFactory_Interface() = default;

    DISABLE_COPY_MOVE(ICodecFactory_Interface)

    virtual unique_ptr<IPackageCodec_Interface> CreateUniquePackageCodec(ATcpSocket socket) = 0;
    virtual unique_ptr<IRecyclerBase>           CreateUniquePackagePool()                   = 0;

    IPackageCodec_Interface *   CreatePackageCodec(ATcpSocket socket);
    IRecyclerBase *             CreatePackagePool();
};