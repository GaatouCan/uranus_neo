#pragma once

#include "Recycle.h"

#include <queue>
#include <concepts>
#include <atomic>
#include <shared_mutex>



/**
 * The Abstract Base Class Of Recycler,
 * Its Subclass Must Be Created By std::make_shared
 */
class BASE_API IRecyclerBase : public std::enable_shared_from_this<IRecyclerBase> {

    /** Internal Container */
    std::queue<unique_ptr<IRecycleInterface>> mQueue;
    mutable std::shared_mutex mMutex;

    std::atomic_int64_t mUsage;

    static constexpr float      RECYCLER_EXPAND_THRESHOLD   = 0.75f;
    static constexpr float      RECYCLER_EXPAND_RATE        = 1.f;

    static constexpr int        RECYCLER_SHRINK_DELAY       = 1;
    static constexpr float      RECYCLER_SHRINK_THRESHOLD   = 0.3f;
    static constexpr float      RECYCLER_SHRINK_RATE        = 0.5f;
    static constexpr int        RECYCLER_MINIMUM_CAPACITY   = 64;

protected:
    IRecyclerBase();
    explicit IRecyclerBase(size_t capacity);

    [[nodiscard]] virtual IRecycleInterface *Create() const = 0;

public:
    virtual ~IRecyclerBase();

    DISABLE_COPY_MOVE(IRecyclerBase)

    void Initial(size_t capacity = RECYCLER_MINIMUM_CAPACITY);

    /** Pop The Element Of The Front Of The Internal Queue */
    shared_ptr<IRecycleInterface> Acquire();
    void Shrink();

    [[nodiscard]] size_t GetUsage() const;
    [[nodiscard]] size_t GetIdle() const;
    [[nodiscard]] size_t GetCapacity() const;

private:
    void Recycle(IRecycleInterface *pElem);
};


template<class Type>
requires std::derived_from<Type, IRecycleInterface> && (!std::is_same_v<Type, IRecycleInterface>)
class TRecycler final : public IRecyclerBase {

protected:
    IRecycleInterface *Create() const override {
        return new Type();
    }

public:
    TRecycler() : IRecyclerBase() { }
    explicit TRecycler(const size_t capacity) : IRecyclerBase(capacity) { }

    shared_ptr<Type> AcquireT() {
        auto res = this->Acquire();
        if (res == nullptr)
            return nullptr;

        return std::dynamic_pointer_cast<Type>(res);
    }
};
