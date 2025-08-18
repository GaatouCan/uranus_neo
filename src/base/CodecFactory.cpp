#include "CodecFactory.h"
#include "PackageCodec.h"
#include "Recycler.h"

IPackageCodec_Interface *ICodecFactory_Interface::CreatePackageCodec(ATcpSocket socket) {
    return this->CreateUniquePackageCodec(std::move(socket)).release();
}

IRecyclerBase *ICodecFactory_Interface::CreatePackagePool() {
    return this->CreateUniquePackagePool().release();
}
