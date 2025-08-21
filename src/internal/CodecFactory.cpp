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

unique_ptr<IPackageCodec_Interface> UCodecFactory::CreateUniquePackageCodec(ATcpSocket socket) {
    return make_unique<UPacketCodec>(ASslStream(std::move(socket), mSSLContext));
}

unique_ptr<IRecyclerBase> UCodecFactory::CreateUniquePackagePool() {
    return IRecyclerBase::CreateUnique<FPacket>();
}

IPackageCodec_Interface *UCodecFactory::CreatePackageCodec(ATcpSocket socket) {
    return new UPacketCodec(ASslStream(std::move(socket), mSSLContext));
}

IRecyclerBase *UCodecFactory::CreatePackagePool() {
    return IRecyclerBase::Create<FPacket>();
}
