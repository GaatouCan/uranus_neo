#include "SharedLibrary.h"

#include <stdexcept>
#include <format>

FSharedLibrary::FSharedLibrary()
    : mControl(nullptr) {
}

FSharedLibrary::~FSharedLibrary() {
    Release();
}

FSharedLibrary::FSharedLibrary(const std::string &path) {
    mControl = new FControlBlock();
    mControl->refCount = 1;

#if defined(_WIN32) || defined(_WIN64)
    mControl->handle = LoadLibrary(path.data());
    if (!mControl->handle) {
        throw std::runtime_error(std::format("{} - Fail To Load Dynamic Library[{}]", __FUNCTION__, path));
    }
#else
    mControl->handle = dlopen(path.data(), RTLD_LAZY);
    if (!mControl->handle) {
        throw std::runtime_error(std::format("{} - Fail To Load Dynamic Library[{}]", __FUNCTION__, path));
    }
#endif
}

FSharedLibrary::FSharedLibrary(const FSharedLibrary &rhs) {
    mControl = rhs.mControl;
    if (mControl) {
        ++mControl->refCount;
    }
}

FSharedLibrary &FSharedLibrary::operator=(const FSharedLibrary &rhs) {
    if (this != &rhs) {
        Release();
        mControl = rhs.mControl;
        if (mControl) {
            ++mControl->refCount;
        }
    }
    return *this;
}

FSharedLibrary::FSharedLibrary(FSharedLibrary &&rhs) noexcept {
    mControl = rhs.mControl;
    rhs.mControl = nullptr;
}

FSharedLibrary &FSharedLibrary::operator=(FSharedLibrary &&rhs) noexcept {
    if (this !=&rhs) {
        Release();
        mControl = rhs.mControl;
        rhs.mControl = nullptr;
    }
    return *this;
}

size_t FSharedLibrary::GetUseCount() const noexcept {
    return mControl ? mControl->refCount.load() : 0;
}

bool FSharedLibrary::IsValid() const noexcept {
    return mControl != nullptr;
}

void FSharedLibrary::Swap(FSharedLibrary &rhs) {
    std::swap(mControl, rhs.mControl);
}

void FSharedLibrary::Reset() {
    FSharedLibrary().Swap(*this);
}


void FSharedLibrary::Release() {
    if (mControl && --mControl->refCount == 0) {
#if defined(_WIN32) || defined(_WIN64)
        FreeLibrary(mControl->handle);
#else
        dlclose(mControl->handle);
#endif

        delete mControl;
    }
    mControl = nullptr;
}
