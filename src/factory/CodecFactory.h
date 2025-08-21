#pragma once

#include "Common.h"
#include "base/Types.h"

#include <memory>


class IRecyclerBase;
class IPackageCodec_Interface;

using std::shared_ptr;
using std::unique_ptr;
using std::make_shared;
using std::make_unique;


class BASE_API ICodecFactory_Interface {
public:
    ICodecFactory_Interface() = default;

    virtual ~ICodecFactory_Interface() = default;

    DISABLE_COPY_MOVE(ICodecFactory_Interface)

    virtual unique_ptr<IPackageCodec_Interface> CreateUniquePackageCodec(ATcpSocket socket) = 0;
    virtual unique_ptr<IRecyclerBase> CreateUniquePackagePool() = 0;

    virtual IPackageCodec_Interface *CreatePackageCodec(ATcpSocket socket) = 0;
    virtual IRecyclerBase *CreatePackagePool() = 0;
};
