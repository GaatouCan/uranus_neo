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

#define ARGS_WRAPPER(...) __VA_ARGS__

#define COMBINE_BODY_MACRO_IMPL(A, B, C, D) A##B##C##D
#define COMBINE_BODY_MACRO(A, B, C, D) COMBINE_BODY_MACRO_IMPL(A, B, C, D)

#define GENERATED_BODY() \
COMBINE_BODY_MACRO(CURRENT_FILE_ID, _, __LINE__, _GENERATED_IMPL)

#define STATIC_CLASS_IMPL(clazz) \
static UGenerated_##clazz *StaticClazz(); \
[[nodiscard]] UClazz *GetClazz() const override;

#define GENERATED_CLAZZ_HEADER(clazz) \
[[nodiscard]] constexpr const char *GetClazzName() const override { return #clazz; } \
static UGenerated_Player &Instance(); \
[[nodiscard]] UObject *Create() const override; \
void Destroy(UObject *obj) const override; \
[[nodiscard]] size_t GetClazzSize() const override;

#define CONSTRUCTOR_TYPE(...) __VA_ARGS__

#define REGISTER_CONSTRUCTOR(clazz, ...) \
RegisterConstructor(new TConstructor<clazz, __VA_ARGS__>());

#define REGISTER_FIELD(clazz, field) \
RegisterField(new TClazzField<clazz, decltype(clazz::field)>(#field, offsetof(clazz, field)));

#define REGISTER_METHOD(clazz, method) \
RegisterMethod(new TClazzMethod(#method, &##clazz##::##method));

inline constexpr auto EmptyCallback = [](auto && ...){};

inline constexpr int PLAYER_TARGET_ID       = -1;
inline constexpr int CLIENT_TARGET_ID       = -2;
inline constexpr int SERVER_SOURCE_ID       = -3;
inline constexpr int INVALID_SERVICE_ID     = -10;

inline constexpr auto CORE_SERVICE_DIRECTORY    = "service";
inline constexpr auto EXTEND_SERVICE_DIRECTORY  = "extend";
inline constexpr auto PLAYER_AGENT_DIRECTORY    = "agent";

inline constexpr int HEARTBEAT_PACKAGE_ID       = 1001;
inline constexpr int LOGIN_REQUEST_PACKAGE_ID   = 1002;
inline constexpr int LOGIN_RESPONSE_PACKAGE_ID  = 1003;
inline constexpr int LOGIN_REPEATED_PACKAGE_ID  = 1004;
inline constexpr int PLATFORM_PACKAGE_ID        = 1005;
inline constexpr int LOGOUT_REQUEST_PACKAGE_ID  = 1006;