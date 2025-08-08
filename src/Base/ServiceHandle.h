#pragma once

#include "Common.h"

#include <cstdint>
#include <xhash>


struct BASE_API FServiceHandle {
    int64_t id = INVALID_SERVICE_ID;

    FServiceHandle() {
    }

    // NOLINT(google-explicit-constructor)
    FServiceHandle(const int64_t id)
        : id(id) {
    }

    // NOLINT(google-explicit-constructor)
    operator int64_t() const {
        return id;
    }

    bool operator<(const FServiceHandle &other) const {
        return id < other.id;
    }

    bool operator>(const FServiceHandle &other) const {
        return id > other.id;
    }
};


inline bool operator==(const FServiceHandle &lhs, const FServiceHandle &rhs) {
    return lhs.id == rhs.id;
}

inline bool operator==(const int lhs, const FServiceHandle &rhs) {
    return lhs == rhs.id;
}

inline bool operator==(const FServiceHandle &lhs, const int rhs) {
    return lhs.id == rhs;
}


inline bool operator<(const FServiceHandle &lhs, const int rhs) {
    return lhs.id < rhs;
}

inline bool operator<(const int lhs, const FServiceHandle &rhs) {
    return lhs < rhs.id;
}

inline bool operator>(const FServiceHandle &lhs, const int rhs) {
    return lhs.id > rhs;
}

inline bool operator>(const int lhs, const FServiceHandle &rhs) {
    return lhs < rhs.id;
}

template<>
struct std::hash<FServiceHandle> {
    size_t operator()(const FServiceHandle &h) const noexcept {
        return std::hash<int64_t>{}(h.id);
    }
};
