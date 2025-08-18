#pragma once

#include "Common.h"

#include <string>
#include <vector>
#include <stdexcept>

#ifdef __linux__
#include <cstdint>
#endif

/**
 * The Wrapper Of std::vector<unsigned char>
 */
class BASE_API FByteArray final {

    std::vector<uint8_t> mByteArray;

public:
    FByteArray() = default;

    explicit FByteArray(size_t size);
    explicit FByteArray(const std::vector<uint8_t> &bytes);

    explicit operator std::vector<uint8_t>() const;

    FByteArray(const FByteArray &rhs)                   = default;
    FByteArray &operator=(const FByteArray &rhs)        = default;
    FByteArray(FByteArray &&rhs) noexcept               = default;
    FByteArray &operator=(FByteArray &&rhs) noexcept    = default;

    /** Clean The Data And Release The Space */
    void Reset();

    void Clear();

    [[nodiscard]] size_t Size() const;

    void Resize (size_t size);
    void Reserve(size_t capacity);

    [[nodiscard]]       uint8_t *   Data();
    [[nodiscard]] const uint8_t *   Data() const;

    std::vector<uint8_t> &RawRef();

    void                        FromString  (std::string_view sv);
    [[nodiscard]] std::string   ToString    ()const;

    auto                Begin()         -> decltype(mByteArray)::iterator;
    auto                End()           -> decltype(mByteArray)::iterator;
    [[nodiscard]] auto  Begin()  const  -> decltype(mByteArray)::const_iterator;
    [[nodiscard]] auto  End()    const  -> decltype(mByteArray)::const_iterator;

    [[nodiscard]] uint8_t operator[](size_t pos) const noexcept;

    template<typename T>
    static constexpr bool kCheckPODType = std::is_pointer_v<T>
        ? std::is_trivial_v<std::remove_pointer_t<T> > && std::is_standard_layout_v<std::remove_pointer_t<T> >
        : std::is_trivial_v<T> && std::is_standard_layout_v<T>;

    template<typename T>
    requires kCheckPODType<T>
    void CastFrom(const T &source) {
        constexpr auto size = std::is_pointer_v<T> ? sizeof(std::remove_pointer_t<T>) : sizeof(T);
        mByteArray.reserve(size);

        const void *src = nullptr;

        if constexpr (std::is_pointer_v<T>) {
            static_assert(!std::is_null_pointer_v<T>, "Null Pointer Pass To CastFrom");
            src = static_cast<const void *>(source);
        } else {
            src = static_cast<const void *>(&source);
        }

        std::memcpy(mByteArray.data(), src, size);
    }

    template<typename T>
    requires kCheckPODType<T>
    void CastFromVector(const std::vector<T> &source) {
        constexpr auto size = std::is_pointer_v<T> ? sizeof(std::remove_pointer_t<T>) : sizeof(T);
        mByteArray.reserve(size * source.size());

        if constexpr (std::is_pointer_v<T>) {
            for (size_t idx = 0; idx < size; idx++) {
                std::memcpy(mByteArray.data() + idx * size, static_cast<const void *>(source[idx]), size);
            }
        } else {
            std::memcpy(mByteArray.data(), static_cast<const void *>(source.data()), size * size);
        }
    }

    template<typename T>
    requires kCheckPODType<T>
    FByteArray &operator <<(const T &source) {
        this->CastFrom(source);
        return *this;
    }

    template<typename T>
    requires kCheckPODType<T>
    FByteArray &operator <<(const std::vector<T> &source) {
        this->CastFromVector(source);
        return *this;
    }

    template<typename T>
    requires kCheckPODType<T>
    void CastTo(T &target) const {
        constexpr auto size = std::is_pointer_v<T> ? sizeof(std::remove_pointer_t<T>) : sizeof(T);
        if (size > mByteArray.size()) {
            throw std::runtime_error("FByteArray::CastTo - Overflow.");
        }

        void *dist = nullptr;

        if constexpr (std::is_pointer_v<T>) {
            static_assert(!std::is_null_pointer_v<T>, "Null Pointer Pass To CastTo");
            dist = static_cast<void *>(target);
        } else {
            dist = static_cast<void *>(&target);
        }

        std::memcpy(dist, mByteArray.data(), size);
    }

    template<typename T>
    requires (!std::is_pointer_v<T>) && std::is_trivial_v<T> && std::is_standard_layout_v<T>
    void CastToVector(std::vector<T> &dist) {
        constexpr auto size = sizeof(T);
        const size_t count = mByteArray.size() / size;
        const size_t length = size * count;

        if (count == 0)
            return;

        dist.reserve(count);

        std::memset(dist.data(), 0, length);
        std::memcpy(dist.data(), mByteArray.data(), length);
    }

    template<typename T>
    requires kCheckPODType<T>
    FByteArray &operator >>(T *target) {
        this->CastTo(target);
        return *this;
    }

    template<typename T>
    requires (!std::is_pointer_v<T>) && std::is_trivial_v<T> && std::is_standard_layout_v<T>
    FByteArray &operator >>(std::vector<T> &dist) {
        this->CastToVector(dist);
        return *this;
    }

    template<typename T>
    requires kCheckPODType<T>
    static FByteArray From(T &&data) {
        FByteArray bytes;
        bytes.CastFrom(std::forward<T>(data));
        return bytes;
    }
};

template<typename T>
requires FByteArray::kCheckPODType<T>
std::vector<uint8_t> DataToByteArray(T data) {
    std::vector<uint8_t> bytes;

    constexpr auto size = std::is_pointer_v<T> ? sizeof(std::remove_pointer_t<T>) : sizeof(T);
    bytes.reserve(size);

    const void *src = nullptr;

    if constexpr (std::is_pointer_v<T>) {
        static_assert(!std::is_null_pointer_v<T>, "DataToByteArray Get Null Pointer");
        src = static_cast<const void *>(data);
    } else {
        src = static_cast<const void *>(&data);
    }

    std::memcpy(bytes.data(), src, size);

    return bytes;
}

template<typename T>
requires FByteArray::kCheckPODType<T>
void ByteArrayToData(const std::vector<uint8_t> &bytes, T &target) {
    constexpr auto size = std::is_pointer_v<T> ? sizeof(std::remove_pointer_t<T>) : sizeof(T);
    if (size > bytes.size()) {
        throw std::runtime_error("FByteArray::CastTo - Overflow.");
    }

    void *dist = nullptr;

    if constexpr (std::is_pointer_v<T>) {
        dist = static_cast<void *>(target);
    } else {
        dist = static_cast<void *>(&target);
    }

    std::memcpy(dist, bytes.data(), size);
}

template<typename T>
requires FByteArray::kCheckPODType<T>
std::vector<uint8_t> VectorToByteArray(const std::vector<T> &list) {
    std::vector<uint8_t> bytes;

    constexpr auto size = std::is_pointer_v<T> ? sizeof(std::remove_pointer_t<T>) : sizeof(T);
    bytes.reserve(size * list.size());

    if constexpr (std::is_pointer_v<T>) {
        for (size_t idx = 0; idx < size; idx++) {
            std::memcpy(bytes.data() + idx * size, static_cast<const void *>(list[idx]), size);
        }
    } else {
        std::memcpy(bytes.data(), static_cast<const void *>(list.data()), size * size);
    }

    return bytes;
}

template<typename T>
requires (!std::is_pointer_v<T>) && std::is_trivial_v<T> && std::is_standard_layout_v<T>
void ByteArrayToVector(const std::vector<uint8_t> &src, std::vector<T> &dist) {
    constexpr auto size = sizeof(T);
    const size_t count  = src.size() / size;
    const size_t length = size * count;

    if (count == 0)
        return;

    dist.reserve(count);

    std::memset(dist.data(), 0, length);
    std::memcpy(dist.data(), static_cast<const void *>(src.data()), length);
}
