#pragma once

#include "Common.h"
#include <memory>


using std::weak_ptr;
using std::shared_ptr;

class UAgent;


struct BASE_API FAgentHandle {

    int64_t id;
    weak_ptr<UAgent> weakPtr;

    FAgentHandle() : id(-1) {

    }

    // NOLINT(google-explicit-constructor)
    FAgentHandle(const int64_t id)
        : id(id) {
    }

    FAgentHandle(const int64_t id, const weak_ptr<UAgent> &ptr)
        : id(id), weakPtr(ptr) {
    }

    bool operator<(const FAgentHandle &rhs) const {
        return id < rhs.id;
    }

    bool operator>(const FAgentHandle &rhs) const {
        return id > rhs.id;
    }

    // NOLINT(google-explicit-constructor)
    operator int64_t() const {
        return id;
    }

    [[nodiscard]] bool IsValid() const {
        return id > 0 && !weakPtr.expired();
    }

    [[nodiscard]] shared_ptr<UAgent> Get() const {
        return weakPtr.lock();
    }

    struct BASE_API FEqual {
        bool operator()(const FAgentHandle &lhs, const FAgentHandle &rhs) const {
            return lhs.id == rhs.id;
        }
    };

    struct BASE_API FHash {
        size_t operator()(const FAgentHandle &handle) const {
            return std::hash<int64_t>()(handle.id);
        }
    };
};


inline bool operator==(const FAgentHandle &lhs, const FAgentHandle &rhs) {
    return lhs.id == rhs.id;
}

inline bool operator==(const int lhs, const FAgentHandle &rhs) {
    return lhs == rhs.id;
}

inline bool operator==(const FAgentHandle &lhs, const int rhs) {
    return lhs.id == rhs;
}

inline bool operator<(const FAgentHandle &lhs, const int rhs) {
    return lhs.id < rhs;
}

inline bool operator<(const int lhs, const FAgentHandle &rhs) {
    return lhs < rhs.id;
}

inline bool operator>(const FAgentHandle &lhs, const int rhs) {
    return lhs.id > rhs;
}

inline bool operator>(const int lhs, const FAgentHandle &rhs) {
    return lhs < rhs.id;
}
