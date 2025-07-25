#pragma once

#include "Common.h"

#include <string>
#include <atomic>
#if defined(_WIN32) || defined(_WIN64)
#include <Windows.h>
using AModuleHandle = HMODULE;
#else
#include <dlfcn.h>
using AModuleHandle = void *;
#endif


class BASE_API FSharedLibrary final {

    struct FControlBlock {
        AModuleHandle handle;
        std::atomic_size_t refCount;
    };

    FControlBlock *mControl;

public:
    FSharedLibrary();
    ~FSharedLibrary();

    explicit FSharedLibrary(const std::string &path);

    FSharedLibrary(const FSharedLibrary &rhs);
    FSharedLibrary &operator=(const FSharedLibrary &rhs);

    FSharedLibrary(FSharedLibrary &&rhs) noexcept;
    FSharedLibrary &operator=(FSharedLibrary &&rhs) noexcept;

private:
    void Release();
};


