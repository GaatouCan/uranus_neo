#pragma once

#include "Base/CodecFactory.h"

#include <asio/ssl/context.hpp>

class BASE_API UCodecFactory final : public ICodecFactory_Interface {

    asio::ssl::context mSSLContext;

public:
    UCodecFactory();
    ~UCodecFactory() override;

    IPackageCodec_Interface *CreateCodec(ATcpSocket socket) override;
};

