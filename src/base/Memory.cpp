#include "Memory.h"

namespace memory {
    bool IRefCountBase::IncRefCount_NotZero() {
        auto count = mRefCount.load(std::memory_order_acquire);
        while (count != 0) {
            if (mRefCount.compare_exchange_weak(
                count,
                count + 1,
                std::memory_order_relaxed,
                std::memory_order_relaxed)) {
                return true;
                }
        }
        return false;
    }

    void IRefCountBase::IncRefCount() {
        mRefCount.fetch_add(1, std::memory_order_relaxed);
    }

    void IRefCountBase::DecRefCount() {
        if (mRefCount.fetch_sub(1, std::memory_order_relaxed) == 1) {
            this->Destroy();
            this->Delete();
        }
    }

    size_t IRefCountBase::GetRefCount() const noexcept {
        return mRefCount.load(std::memory_order_acquire);
    }

    void ISharedCountBase::DecRefCount() {
        if (mRefCount.fetch_sub(1, std::memory_order_relaxed) == 1) {
            this->Destroy();
            this->DecWeakCount();
        }
    }

    void ISharedCountBase::IncWeakCount() {
        mWeakCount.fetch_add(1, std::memory_order_relaxed);
    }

    void ISharedCountBase::DecWeakCount() {
        if (mWeakCount.fetch_sub(1, std::memory_order_relaxed) == 1) {
            this->Delete();
        }
    }
}
