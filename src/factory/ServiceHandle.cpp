#include "ServiceHandle.h"
#include "ServiceFactory.h"
#include "service/ServiceBase.h"


using AServiceCreator   = IServiceBase *(*)();
using AServiceDestroyer = void (*)(IServiceBase *);


FServiceHandle::FServiceHandle()
    : mService(nullptr) {
}

FServiceHandle::FServiceHandle(const FSharedLibrary &library)
    : mService(nullptr),
      mLibrary(library) {

    auto creator = mLibrary.GetSymbol<AServiceCreator>("CreateInstance");
    auto destroyer = mLibrary.GetSymbol<AServiceDestroyer>("DestroyInstance");

    if (creator == nullptr || destroyer == nullptr)
        return;

    mService = std::invoke(creator);
}


FServiceHandle::~FServiceHandle() {
    Release();
}

FServiceHandle::FServiceHandle(FServiceHandle &&rhs) noexcept {
    mService = rhs.mService;
    rhs.mService = nullptr;

    mLibrary = std::move(rhs.mLibrary);
}

FServiceHandle &FServiceHandle::operator=(FServiceHandle &&rhs) noexcept {
    if (this != &rhs) {
        mService = rhs.mService;
        rhs.mService = nullptr;

        mLibrary = std::move(rhs.mLibrary);
    }
    return *this;
}

bool FServiceHandle::IsValid() const {
    return mService != nullptr;
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
    return mService == rhs.mService && mLibrary == rhs.mLibrary;
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

    if (mLibrary.IsValid()) {
        delete mService;
        return;
    }

    auto destroyer = mLibrary.GetSymbol<AServiceDestroyer>("DestroyInstance");
    if (destroyer != nullptr) {
        std::invoke(destroyer, mService);
    } else {
        delete mService;
    }

    mService = nullptr;
    mLibrary.Reset();
}
