#include "CodecFactory.h"

IRecyclerBase *ICodecFactory_Interface::CreatePackagePool() {
    return this->CreateUniquePackagePool().release();
}