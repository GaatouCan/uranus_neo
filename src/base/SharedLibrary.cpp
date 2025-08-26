#include "SharedLibrary.h"

#include <stdexcept>
#include <format>

FSharedLibrary::FSharedLibrary()
    : mControl(nullptr),
      mHandle(nullptr){
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

    mHandle = mControl->handle;
}

FSharedLibrary::FSharedLibrary(const std::filesystem::path &path)
    : FSharedLibrary(path.string()) {
}

FSharedLibrary::FSharedLibrary(const FSharedLibrary &rhs) {
    mControl = rhs.mControl;
    mHandle = rhs.mHandle;
    if (mControl) {
        ++mControl->refCount;
    }
}

FSharedLibrary &FSharedLibrary::operator=(const FSharedLibrary &rhs) {
    if (this != &rhs) {
        Release();
        mControl = rhs.mControl;
        mHandle = rhs.mHandle;

        if (mControl) {
            ++mControl->refCount;
        }
    }
    return *this;
}

FSharedLibrary::FSharedLibrary(FSharedLibrary &&rhs) noexcept {
    mControl = rhs.mControl;
    mHandle = rhs.mHandle;
    rhs.mControl = nullptr;
    rhs.mHandle = nullptr;
}

FSharedLibrary &FSharedLibrary::operator=(FSharedLibrary &&rhs) noexcept {
    if (this != &rhs) {
        Release();
        mControl = rhs.mControl;
        mHandle = rhs.mHandle;
        rhs.mControl = nullptr;
        rhs.mHandle = nullptr;
    }
    return *this;
}

size_t FSharedLibrary::GetUseCount() const noexcept {
    return mControl ? mControl->refCount.load() : 0;
}

bool FSharedLibrary::IsValid() const noexcept {
    return mControl != nullptr && mHandle != nullptr;
}

FSharedLibrary::operator bool() const noexcept {
    return IsValid();
}

void FSharedLibrary::Swap(FSharedLibrary &rhs) {
    std::swap(mControl, rhs.mControl);
    std::swap(mHandle, rhs.mHandle);
}

void FSharedLibrary::Reset() {
    FSharedLibrary().Swap(*this);
}

bool FSharedLibrary::operator==(const FSharedLibrary &rhs) const {
    return mControl == rhs.mControl;
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
    mHandle = nullptr;
}
