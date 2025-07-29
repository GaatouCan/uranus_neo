#pragma once

#if defined(_WIN32) || defined(_WIN64)
    #ifdef BUILD_IMPL_LIBRARY
        #define IMPL_API __declspec(dllexport)
    #else
        #define IMPL_API __declspec(dllimport)
    #endif
#else
    #define IMPL_API __attribute__((visibility("default")))
#endif
