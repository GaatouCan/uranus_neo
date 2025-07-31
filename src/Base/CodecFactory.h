#pragma once

#include "Common.h"
#include "Types.h"

class IPackageCodec_Interface;

class BASE_API ICodecFactory_Interface {

public:
    ICodecFactory_Interface() = default;
    virtual ~ICodecFactory_Interface() = default;

    DISABLE_COPY_MOVE(ICodecFactory_Interface)

    virtual IPackageCodec_Interface *CreateCodec(ATcpSocket socket) = 0;
};