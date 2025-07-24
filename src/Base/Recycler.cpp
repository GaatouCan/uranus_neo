#include "Recycler.h"

#include <spdlog/spdlog.h>
#ifdef __linux__
#include <mutex>
#endif


IRecyclerBase::IRecyclerBase()
    : mUsage(-1) {
    static_assert(RECYCLER_SHRINK_RATE > RECYCLER_SHRINK_THRESHOLD);
}

IRecyclerBase::IRecyclerBase(const size_t capacity)
    : IRecyclerBase() {
    Initial(capacity);
}

IRecyclerBase::~IRecyclerBase() {
}

std::shared_ptr<IRecycleInterface> IRecyclerBase::Acquire() {
    // Not Initialized
    if (mUsage < 0)
        return nullptr;

    // Custom Deleter Of The Smart Pointer
    auto deleter = [weak = weak_from_this()](IRecycleInterface *pElem) {
        if (const auto self = weak.lock()) {
            self->Recycle(pElem);
        } else {
            delete pElem;
        }
    };

    // Pop The Front From The Queue If It Is Not Empty
    {
        std::unique_lock lock(mMutex);
        if (!mQueue.empty()) {
            auto *pElem = mQueue.front().release();
            mQueue.pop();

            SPDLOG_TRACE("{:<20} - Recycler[{:p}] - Acquire Recyclable[{:p}] From Queue",
                         __FUNCTION__, static_cast<void *>(this), static_cast<void *>(pElem));

            pElem->Initial();
            ++mUsage;

            return {pElem, deleter};
        }
    }

    // Calculate How Many New Elements Need To Be Created
    size_t num = static_cast<size_t>(static_cast<float>(mUsage.load()) * RECYCLER_EXPAND_RATE);

    if (num <= 0) {
        SPDLOG_ERROR("{:<20} - Recycler[{:p}] - Inner Queue Is Empty But Usage Equals Zero!");
        return nullptr;
    }

    // The Last One Directly Return
    std::vector<IRecycleInterface *> elems(num - 1);
    IRecycleInterface *pResult = nullptr;

    while (num-- > 0) {
        auto *pElem = Create();

        if (pElem == nullptr)
            continue;

        pElem->OnCreate();

        // If The Last One, As The Result
        if (num == 1) {
            pResult = pElem;
            continue;
        }

        elems.emplace_back(pElem);
    }

    if (pResult == nullptr) {
        SPDLOG_ERROR("{:<20} - Recycler[{:p}] - Expand Failed!");
        return nullptr;
    }

    // Emplace To The Internal Queue
    if (!elems.empty()) {
        std::unique_lock lock(mMutex);
        for (const auto &pElem: elems) {
            mQueue.emplace(pElem);
        }
    }

    pResult->Initial();
    ++mUsage;

    return {pResult, deleter};
}

size_t IRecyclerBase::GetUsage() const {
    if (mUsage < 0)
        return 0;
    return mUsage;
}

size_t IRecyclerBase::GetIdle() const {
    if (mUsage < 0)
        return 0;

    std::shared_lock lock(mMutex);
    return mQueue.size();
}

size_t IRecyclerBase::GetCapacity() const {
    if (mUsage < 0)
        return 0;

    std::shared_lock lock(mMutex);
    return mQueue.size() + mUsage.load();
}

void IRecyclerBase::Shrink() {
    size_t num = 0;

    // Check If It Needs To Shrink
    {
        std::shared_lock lock(mMutex);

        // Recycler Total Capacity
        const size_t total = mQueue.size() + mUsage.load();

        // Usage Less Than Shrink Threshold
        if (static_cast<float>(mUsage.load()) < std::ceil(static_cast<float>(total) * RECYCLER_SHRINK_THRESHOLD)) {
            num = static_cast<size_t>(std::floor(static_cast<float>(total) * RECYCLER_SHRINK_RATE));

            const auto rest = total - num;
            if (rest <= 0) {
                return;
            }

            if (rest < RECYCLER_MINIMUM_CAPACITY) {
                num = total - RECYCLER_MINIMUM_CAPACITY;
            }
        }
    }

    if (num > 0) {
        SPDLOG_TRACE("{:<20} - Recycler[{:p}] Need To Release {} Elements",
            __FUNCTION__, static_cast<void *>(this), num);

        std::unique_lock lock(mMutex);

        while (num-- > 0 && !mQueue.empty()) {
            const auto elem = std::move(mQueue.front());
            mQueue.pop();
        }

        SPDLOG_TRACE("{:<20} - Recycler[{:p}] Shrink Finished",
            __FUNCTION__, static_cast<void *>(this));
    }
}

void IRecyclerBase::Recycle(IRecycleInterface *pElem) {
    if (mUsage < 0) {
        delete pElem;
        return;
    }

    pElem->Reset();
    --mUsage;


    std::unique_lock lock(mMutex);
    mQueue.emplace(pElem);

    SPDLOG_TRACE("{:<20} - Recycler[{:p}] - Recycle Recyclable[{:p}] To Queue",
        __FUNCTION__, static_cast<void *>(this), static_cast<void *>(pElem));
}

void IRecyclerBase::Initial(const size_t capacity) {
    if (mUsage >= 0)
        return;

    for (size_t count = 0; count < capacity; count++) {
        auto *pElem = Create();
        pElem->OnCreate();

        mQueue.emplace(pElem);
    }

    mUsage = 0;
    SPDLOG_TRACE("{:<20} - Recycler[{:p}] - Capacity[{}]", __FUNCTION__, static_cast<void *>(this), capacity);
}
