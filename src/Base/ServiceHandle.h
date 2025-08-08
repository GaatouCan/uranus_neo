#pragma once

#include "Common.h"

#include <cstdint>
#include <xhash>

struct BASE_API FServiceHandle {
    int64_t id = INVALID_SERVICE_ID;

    bool operator<(const FServiceHandle& other) const {
        return id < other.id;
    }

    explicit operator int64_t() const {
        return id;
    }
};

inline bool operator==(const FServiceHandle& lhs, const FServiceHandle& rhs) {
    return lhs.id == rhs.id;
}

template <>
struct std::hash<FServiceHandle> {
    size_t operator()(const FServiceHandle& h) const noexcept {
        return std::hash<int64_t>{}(h.id);
    }
};
