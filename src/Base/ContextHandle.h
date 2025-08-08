#pragma once

#include "ServiceHandle.h"

#include <memory>


class UContextBase;


struct BASE_API FContextHandle {
    FServiceHandle sid;
    std::weak_ptr<UContextBase> wPtr;

    FContextHandle() {}

    // NOLINT(google-explicit-constructor)
    FContextHandle(const FServiceHandle sid) : sid(sid) {}

    FContextHandle(const FServiceHandle sid, const std::weak_ptr<UContextBase> &ptr) : sid(sid), wPtr(ptr) {}

    bool operator<(const FContextHandle &rhs) const {
        return sid < rhs.sid;
    }

    bool operator>(const FContextHandle &rhs) const {
        return sid > rhs.sid;
    }

    // NOLINT(google-explicit-constructor)
    operator int64_t() const {
        return sid;
    }
};


inline bool operator==(const FContextHandle &lhs, const FContextHandle &rhs) {
    return lhs.sid == rhs.sid;
}

inline bool operator==(const int lhs, const FContextHandle &rhs) {
    return lhs == rhs.sid;
}

inline bool operator==(const FContextHandle &lhs, const int rhs) {
    return lhs.sid == rhs;
}

inline bool operator<(const FContextHandle& lhs, const int rhs) {
    return lhs.sid < rhs;
}

inline bool operator<(const int lhs, const FContextHandle& rhs) {
    return lhs < rhs.sid;
}

inline bool operator>(const FContextHandle& lhs, const int rhs) {
    return lhs.sid > rhs;
}

inline bool operator>(const int lhs, const FContextHandle& rhs) {
    return lhs < rhs.sid;
}

template <>
struct std::hash<FContextHandle> {
    size_t operator()(const FContextHandle& h) const noexcept {
        return std::hash<int64_t>{}(h.sid.id);
    }
};