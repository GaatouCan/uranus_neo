#pragma once

#include "factory/CodecFactory.h"

#include <asio/ssl/stream.hpp>
#include <asio/ssl/context.hpp>


class BASE_API UCodecFactory final : public ICodecFactory_Interface {
    using ASslStream = asio::ssl::stream<ATcpSocket>;

    asio::ssl::context mSSLContext;

public:
    UCodecFactory();

    unique_ptr<IPackageCodec_Interface> CreateUniquePackageCodec(ATcpSocket socket) override;
    unique_ptr<IRecyclerBase> CreateUniquePackagePool() override;

    IPackageCodec_Interface *CreatePackageCodec(ATcpSocket socket) override;
    IRecyclerBase *CreatePackagePool() override;
};
