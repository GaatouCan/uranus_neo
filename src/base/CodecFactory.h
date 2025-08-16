#pragma once

#include "Common.h"
#include "Types.h"

namespace recycle {
    class IRecyclerBase;
}
using recycle::IRecyclerBase;

class IPackageCodec_Interface;


class BASE_API ICodecFactory_Interface {

public:
    ICodecFactory_Interface() = default;
    virtual ~ICodecFactory_Interface() = default;

    DISABLE_COPY_MOVE(ICodecFactory_Interface)

    virtual IPackageCodec_Interface *CreatePackageCodec(ATcpSocket socket) = 0;
    virtual IRecyclerBase *CreatePackagePool() = 0;
};