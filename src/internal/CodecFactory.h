#pragma once

#include "base/CodecFactory.h"

#include <asio/ssl/stream.hpp>
#include <asio/ssl/context.hpp>

class BASE_API UCodecFactory final : public ICodecFactory_Interface {

    using ASslStream = asio::ssl::stream<ATcpSocket>;

    asio::ssl::context mSSLContext;

public:
    UCodecFactory();
    ~UCodecFactory() override;

    IPackageCodec_Interface *CreatePackageCodec(ATcpSocket socket) override;
    IRecyclerBase *CreatePackagePool() override;
};

