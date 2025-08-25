#include "ServiceHandle.h"
#include "ServiceFactory.h"
#include "service/ServiceBase.h"

FServiceHandle::FServiceHandle()
    : mService(nullptr),
      mFactory(nullptr) {
}

FServiceHandle::FServiceHandle(IServiceBase *pService, IServiceFactory_Interface *pFactory)
    : mService(pService),
      mFactory(pFactory) {
}

FServiceHandle::~FServiceHandle() {
    if (!mService)
        return;

    if (!mFactory) {
        delete mService;
        return;
    }

    mFactory->DestroyInstance(mService);
}

FServiceHandle::FServiceHandle(FServiceHandle &&rhs) noexcept {
    mService = rhs.mService;
    mFactory = rhs.mFactory;

    rhs.mService = nullptr;
    rhs.mFactory = nullptr;
}

FServiceHandle &FServiceHandle::operator=(FServiceHandle &&rhs) noexcept {
    if (this != &rhs) {
        mService = rhs.mService;
        mFactory = rhs.mFactory;

        rhs.mService = nullptr;
        rhs.mFactory = nullptr;
    }
    return *this;
}
