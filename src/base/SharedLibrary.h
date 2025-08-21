#pragma once

#include "Common.h"

#include <string>
#include <atomic>
#include <filesystem>
#if defined(_WIN32) || defined(_WIN64)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
using AModuleHandle = HMODULE;
#else
#include <dlfcn.h>
using AModuleHandle = void *;
#endif


class BASE_API FSharedLibrary final {

    struct FControlBlock {
        AModuleHandle       handle;
        std::atomic_size_t  refCount;
    };

    FControlBlock *mControl;

public:
    FSharedLibrary();
    ~FSharedLibrary();

    explicit FSharedLibrary(const std::string &path);
    explicit FSharedLibrary(const std::filesystem::path &path);

    FSharedLibrary(const FSharedLibrary &rhs);
    FSharedLibrary &operator=(const FSharedLibrary &rhs);

    FSharedLibrary(FSharedLibrary &&rhs) noexcept;
    FSharedLibrary &operator=(FSharedLibrary &&rhs) noexcept;

    template<typename Type>
    Type GetSymbol(const std::string &name) const;

    [[nodiscard]] size_t    GetUseCount()   const noexcept;
    [[nodiscard]] bool      IsValid()       const noexcept;

    explicit operator bool() const noexcept;

    void Swap(FSharedLibrary &rhs);
    void Reset();

private:
    void Release();
};


template<typename Type>
inline Type FSharedLibrary::GetSymbol(const std::string &name) const {
    if (mControl->handle == nullptr)
        return nullptr;

#if defined(_WIN32) || defined(_WIN64)
    return reinterpret_cast<Type>(GetProcAddress(mControl->handle, name.c_str()));
#else
    return reinterpret_cast<Type>(dyslm(mControl->handle, name.c_str()));
#endif
}
