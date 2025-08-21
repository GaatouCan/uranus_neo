#pragma once

#include "Common.h"

#include <memory>


class UTimer;
using std::shared_ptr;
using std::weak_ptr;


struct BASE_API FTimerHandle final {
    int64_t id;
    std::weak_ptr<UTimer> timer;

    FTimerHandle() : id(-1) {
        
    }

    // NOLINT(google-explicit-constructor)
    FTimerHandle(const int64_t id)
        : id(id) {
    }

    FTimerHandle(const int64_t id, const weak_ptr<UTimer> &ptr)
        : id(id), timer(ptr) {
    }

    bool operator<(const FTimerHandle &rhs) const {
        return id < rhs.id;
    }

    bool operator>(const FTimerHandle &rhs) const {
        return id > rhs.id;
    }

    // NOLINT(google-explicit-constructor)
    operator int64_t() const {
        return id;
    }

    // NOLINT(google-explicit-constructor)
    operator int() const {
        return static_cast<int>(id);
    }

    [[nodiscard]] bool IsValid() const {
        return id > 0 && !timer.expired();
    }

    [[nodiscard]] shared_ptr<UTimer> Get() const {
        return timer.lock();
    }

    struct BASE_API FEqual {
        bool operator()(const FTimerHandle &lhs, const FTimerHandle &rhs) const {
            return lhs.id == rhs.id;
        }
    };

    struct BASE_API FHash {
        size_t operator()(const FTimerHandle &handle) const {
            return std::hash<int64_t>()(handle.id);
        }
    };
};

inline bool operator==(const FTimerHandle &lhs, const FTimerHandle &rhs) {
    return lhs.id == rhs.id;
}

inline bool operator==(const int lhs, const FTimerHandle &rhs) {
    return lhs == rhs.id;
}

inline bool operator==(const FTimerHandle &lhs, const int rhs) {
    return lhs.id == rhs;
}


inline bool operator<(const FTimerHandle &lhs, const int rhs) {
    return lhs.id < rhs;
}

inline bool operator<(const int lhs, const FTimerHandle &rhs) {
    return lhs < rhs.id;
}

inline bool operator>(const FTimerHandle &lhs, const int rhs) {
    return lhs.id > rhs;
}

inline bool operator>(const int lhs, const FTimerHandle &rhs) {
    return lhs < rhs.id;
}
