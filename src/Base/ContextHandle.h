#pragma once

#include "ServiceHandle.h"

#include <memory>


class UContextBase;


struct BASE_API FContextHandle {
    FServiceHandle sid;
    std::weak_ptr<UContextBase> wPtr;

    bool operator<(const FContextHandle &rhs) const {
        return sid < rhs.sid;
    }

    explicit operator int64_t() const {
        return static_cast<int64_t>(sid);
    }
};


inline bool operator==(const FContextHandle &lhs, const FContextHandle &rhs) {
    return lhs.sid == rhs.sid;
}


template <>
struct std::hash<FContextHandle> {
    size_t operator()(const FContextHandle& h) const noexcept {
        return std::hash<int64_t>{}(h.sid.id);
    }
};