#include "ServiceHandle.h"
#include "ServiceFactory.h"
#include "service/ServiceBase.h"


FServiceHandle::FServiceHandle()
    : mService(nullptr),
      mFactory(nullptr) {
}

FServiceHandle::FServiceHandle(IServiceBase *pService, IServiceFactory_Interface *pFactory, const std::string &path)
    : mService(pService),
      mFactory(pFactory),
      mPath(path) {
}


FServiceHandle::~FServiceHandle() {
    Release();
}

FServiceHandle::FServiceHandle(FServiceHandle &&rhs) noexcept {
    mService = rhs.mService;
    mFactory = rhs.mFactory;

    rhs.mService = nullptr;
    rhs.mFactory = nullptr;

    mPath = std::move(rhs.mPath);
}

FServiceHandle &FServiceHandle::operator=(FServiceHandle &&rhs) noexcept {
    if (this != &rhs) {
        Release();

        mService = rhs.mService;
        mFactory = rhs.mFactory;

        rhs.mService = nullptr;
        rhs.mFactory = nullptr;

        mPath = std::move(rhs.mPath);
    }
    return *this;
}

const std::string &FServiceHandle::GetPath() const {
    return mPath;
}

bool FServiceHandle::IsValid() const {
    return mService != nullptr && mFactory != nullptr && !mPath.empty();
}

IServiceBase *FServiceHandle::operator->() const noexcept {
    return mService;
}

IServiceBase &FServiceHandle::operator*() const noexcept {
    return *mService;
}

IServiceBase *FServiceHandle::Get() const noexcept {
    return mService;
}

bool FServiceHandle::operator==(const FServiceHandle &rhs) const noexcept {
    return mService == rhs.mService && mFactory == rhs.mFactory &&
            mPath == rhs.mPath;
}

bool FServiceHandle::operator==(nullptr_t) const noexcept {
    return mService == nullptr;
}

FServiceHandle::operator bool() const noexcept {
    return IsValid();
}

void FServiceHandle::Release() {
    if (!mService)
        return;

    if (mFactory != nullptr && !mPath.empty()) {
        mFactory->DestroyInstance(mService, mPath);
    } else {
        delete mService;
    }

    mService = nullptr;
    mFactory = nullptr;
    mPath.clear();
}
