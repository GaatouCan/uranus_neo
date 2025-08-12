#pragma once

#include <unordered_set>
#include <atomic>
#include <mutex>

template<class Type, bool bConcurrent>
requires std::is_integral_v<Type>
class TIdentAllocator {

    struct FEmptyMutex {};

    using AAllocatorMutex = std::conditional_t<bConcurrent, std::mutex, FEmptyMutex>;
    using AIntegralType = std::conditional_t<bConcurrent, std::atomic<Type>, Type>;

public:
    Type AllocateTS();
    Type Allocate();

    void RecycleTS(Type id);
    void Recycle(Type id);

    Type GetUsage() const;

private:
    std::unordered_set<Type> mHashSet;
    AAllocatorMutex mMutex;
    AIntegralType mNext;
    AIntegralType mUsage;
};

template<class Type, bool bConcurrent> requires std::is_integral_v<Type>
inline Type TIdentAllocator<Type, bConcurrent>::AllocateTS() {
    if constexpr (bConcurrent) {
        std::unique_lock lock(mMutex);
        if (const auto iter = mHashSet.begin(); iter != mHashSet.end()) {
            const auto res = *iter;
            mHashSet.erase(iter);

            ++mUsage;
            return res;
        }
    } else {
        if (const auto iter = mHashSet.begin(); iter != mHashSet.end()) {
            const auto res = *iter;
            mHashSet.erase(iter);

            ++mUsage;
            return res;
        }
    }

    ++mUsage;
    return ++mNext;
}

template<class Type, bool bConcurrent> requires std::is_integral_v<Type>
Type TIdentAllocator<Type, bConcurrent>::Allocate() {
    if (const auto iter = mHashSet.begin(); iter != mHashSet.end()) {
        const auto res = *iter;
        mHashSet.erase(iter);

        ++mUsage;
        return res;
    }

    ++mUsage;
    return ++mNext;
}

template<class Type, bool bConcurrent> requires std::is_integral_v<Type>
inline void TIdentAllocator<Type, bConcurrent>::RecycleTS(Type id) {
    if constexpr (bConcurrent) {
        std::unique_lock lock(mMutex);
        mHashSet.emplace(id);
    } else {
        mHashSet.emplace(id);
    }

    --mUsage;
    if constexpr (bConcurrent) {
        mUsage = mUsage.load() > 0 ? mUsage.load() : 0;
    } else {
        mUsage = mUsage > 0 ? mUsage : 0;
    }
}

template<class Type, bool bConcurrent> requires std::is_integral_v<Type>
inline void TIdentAllocator<Type, bConcurrent>::Recycle(Type id) {
    mHashSet.emplace(id);

    --mUsage;
    if constexpr (bConcurrent) {
        mUsage = mUsage.load() > 0 ? mUsage.load() : 0;
    } else {
        mUsage = mUsage > 0 ? mUsage : 0;
    }
}

template<class Type, bool bConcurrent> requires std::is_integral_v<Type>
inline Type TIdentAllocator<Type, bConcurrent>::GetUsage() const {
    if constexpr (bConcurrent) {
        return mUsage.load();
    }
    return mUsage;
}
