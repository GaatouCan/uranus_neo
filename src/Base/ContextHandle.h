#pragma once

#include "ServiceHandle.h"

#include <memory>


using std::shared_ptr;
using std::weak_ptr;

class UContextBase;


struct BASE_API FContextHandle {
    FServiceHandle sid;
    weak_ptr<UContextBase> weakPtr;

    FContextHandle() = default;

    // NOLINT(google-explicit-constructor)
    FContextHandle(const FServiceHandle sid)
        : sid(sid) {
    }

    FContextHandle(const FServiceHandle sid, const weak_ptr<UContextBase> &ptr)
        : sid(sid), weakPtr(ptr) {
    }

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

    [[nodiscard]] bool IsValid() const {
        return sid.IsValid() && !weakPtr.expired();
    }

    [[nodiscard]] shared_ptr<UContextBase> Get() const {
        return weakPtr.lock();
    }

    struct BASE_API FEqual {
        bool operator()(const FContextHandle &lhs, const FContextHandle &rhs) const {
            return lhs.sid == rhs.sid;
        }
    };

    struct BASE_API FHash {
        size_t operator()(const FContextHandle &handle) const {
            return std::hash<int64_t>()(handle.sid.id);
        }
    };
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

inline bool operator<(const FContextHandle &lhs, const int rhs) {
    return lhs.sid < rhs;
}

inline bool operator<(const int lhs, const FContextHandle &rhs) {
    return lhs < rhs.sid;
}

inline bool operator>(const FContextHandle &lhs, const int rhs) {
    return lhs.sid > rhs;
}

inline bool operator>(const int lhs, const FContextHandle &rhs) {
    return lhs < rhs.sid;
}
