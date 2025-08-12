#include "CodecFactory.h"
#include "PacketCodec.h"
#include "base/Recycler.h"

UCodecFactory::UCodecFactory()
    : mSSLContext(asio::ssl::context::tlsv13_server) {
    mSSLContext.use_certificate_chain_file("server.crt");
    mSSLContext.use_private_key_file("server.key", asio::ssl::context::pem);
    mSSLContext.set_options(
        asio::ssl::context::no_sslv2 |
        asio::ssl::context::no_sslv3 |
        asio::ssl::context::default_workarounds |
        asio::ssl::context::single_dh_use
    );
}

UCodecFactory::~UCodecFactory() {
}

IPackageCodec_Interface *UCodecFactory::CreatePackageCodec(ATcpSocket socket) {
    return new UPacketCodec(ASslStream(std::move(socket), mSSLContext));
}

std::shared_ptr<IRecyclerBase> UCodecFactory::CreatePackagePool() {
    return make_shared<TRecycler<FPacket>>();
}
