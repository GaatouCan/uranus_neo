#pragma once

#define REFLECT_API
// #if defined(_WIN32) || defined(_WIN64)
//     #ifdef RTTR_EXPORT
//         #define REFLECT_API __declspec(dllexport)
//     #else
//         #define REFLECT_API __declspec(dllimport)
//     #endif
// #else
//     #define REFLECT_API __attribute__((visibility("default")))
// #endif


#ifdef _DEBUG
    #define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#else
    #define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO
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