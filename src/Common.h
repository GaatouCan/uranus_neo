#pragma once

#if defined(_WIN32) || defined(_WIN64)
    #ifdef URANUS_EXPORT
        #define BASE_API __declspec(dllexport)
    #else
        #define BASE_API __declspec(dllimport)
    #endif
#else
    #define BASE_API __attribute__((visibility("default")))
#endif

#if defined(_WIN32) || defined(_WIN64)
    #ifdef URANUS_SERVICE
        #define SERVICE_API __declspec(dllexport)
    #else
        #define SERVICE_API __declspec(dllimport)
    #endif
#else
    #define SERVICE_API __attribute__((visibility("default")))
#endif


#ifdef _DEBUG
    #define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#else
    #define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO
#endif

#ifdef _DEBUG
#define URANUS_WATCHDOG
#endif

#define DISABLE_COPY(clazz) \
clazz(const clazz&) = delete; \
clazz &operator=(const clazz&) = delete;

#define DISABLE_MOVE(clazz) \
clazz(clazz &&) noexcept = delete; \
clazz &operator=(clazz &&) noexcept = delete;

#define DISABLE_COPY_MOVE(clazz) \
DISABLE_COPY(clazz) \
DISABLE_MOVE(clazz)

#define SPDLOG_NO_SOURCE_LOC

inline constexpr int PLAYER_AGENT_ID = -1;
inline constexpr int CLIENT_TARGET_ID = -2;
inline constexpr int SERVER_SOURCE_ID = -3;
inline constexpr int INVALID_SERVICE_ID = -10;
inline constexpr int MAX_CONTEXT_QUEUE_LENGTH = 10'000;

inline constexpr auto LOGIN_HANDLER_PATH = "login";
inline constexpr auto CORE_SERVICE_DIRECTORY = "service";
inline constexpr auto EXTEND_SERVICE_DIRECTORY = "extend";
inline constexpr auto PLAYER_AGENT_DIRECTORY = "agent";