#pragma once

#include "Common.h"

#include <memory>


class UContextBase;


struct BASE_API FContextHandle {
    int64_t sid;
    std::weak_ptr<UContextBase> wPtr;

    bool operator<(const FContextHandle &rhs) const {
        return sid < rhs.sid;
    }

    struct BASE_API FHash {
        size_t operator()(const FContextHandle &k) const {
            return std::hash<int64_t>()(k.sid);
        }
    };

    struct BASE_API FEqual {
        bool operator()(const FContextHandle &lhs, const FContextHandle &rhs) const {
            return lhs.sid == rhs.sid;
        }
    };
};