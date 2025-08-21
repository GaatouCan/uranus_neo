#include "Recycler.h"

#include <cassert>
#include <spdlog/spdlog.h>


namespace detail {
    FControlBlock::FControlBlock(IRecyclerBase *pRecycler)
        : mRefCount(1),
          mRecycler(pRecycler) {
        SPDLOG_DEBUG("Create Control Block");
    }

    FControlBlock::~FControlBlock() {
        SPDLOG_DEBUG("Destroy Control Block");
    }

    void FControlBlock::Release() {
        mRecycler.store(nullptr, std::memory_order_release);
    }

    bool FControlBlock::IsValid() const {
        return mRecycler.load(std::memory_order_acquire) != nullptr;
    }

    IRecyclerBase *FControlBlock::Get() const {
        return mRecycler.load(std::memory_order_acquire);
    }

    void FControlBlock::IncRefCount() {
        mRefCount.fetch_add(1, std::memory_order_relaxed);
        SPDLOG_TRACE("{} - Current Count[{}]", __FUNCTION__, mRefCount.load());
    }

    void FControlBlock::DecRefCount() {
        if (mRefCount.fetch_sub(1, std::memory_order_relaxed) == 1) {
            if (mRecycler.load(std::memory_order_acquire) == nullptr) {
                delete this;
                return;
            }
        }

        SPDLOG_TRACE("{} - Current Count[{}]", __FUNCTION__, mRefCount.load());
    }

    int64_t FControlBlock::GetRefCount() const {
        return mRefCount.load(std::memory_order_acquire);
    }

    IElementNodeBase::IElementNodeBase(FControlBlock *pCtrl)
        : mRefCount(RECYCLED_REFERENCE_COUNT),
          mControl(pCtrl) {
        // If Control Block Is Null, Throw The Exception
        if (!mControl) [[unlikely]]
            throw std::invalid_argument("Control Block Is Null");

        mControl->IncRefCount();
        SPDLOG_DEBUG("Create Element Node");
    }

    IElementNodeBase::~IElementNodeBase() {
        // Control Block Should Never Be Null
        assert(mControl != nullptr);
        mControl->DecRefCount();

        SPDLOG_DEBUG("Destroy Element Node");
    }

    void IElementNodeBase::OnAcquire() {
        if (int64_t expected = RECYCLED_REFERENCE_COUNT;
            !mRefCount.compare_exchange_strong(
                expected, 1,
                std::memory_order_acq_rel,
                std::memory_order_acquire)) {
            throw std::runtime_error("Acquire on non-recycled node");
        }
        SPDLOG_TRACE("{}", __FUNCTION__);
    }

    void IElementNodeBase::OnRecycle() {
        mRefCount.store(RECYCLED_REFERENCE_COUNT, std::memory_order_relaxed);
        SPDLOG_TRACE("{}", __FUNCTION__);
    }

    void IElementNodeBase::IncRefCount() {
        int64_t cur = mRefCount.load(std::memory_order_acquire);
        for (;;) {
            // Don't Increase The Node In Recycler's Queue
            assert(cur >= 0);
            if (cur < 0)
                throw std::runtime_error("Increase While Recycled");

            if (mRefCount.compare_exchange_weak(
                cur, cur + 1,
                std::memory_order_acq_rel,
                std::memory_order_acquire)) {
                break;
            }
        }
        SPDLOG_TRACE("{} - Current Count[{}]", __FUNCTION__, mRefCount.load());
    }

    bool IElementNodeBase::DecRefCount() {
        SPDLOG_TRACE("{} - Current Count[{}]", __FUNCTION__, mRefCount.load() - 1);
        return mRefCount.fetch_sub(1, std::memory_order_relaxed) == 1;
    }

    IRecyclerBase *IElementNodeBase::GetRecycler() const noexcept {
        assert(mControl != nullptr);
        return mControl->Get();
    }
}

IRecyclerBase::IRecyclerBase(asio::io_context &ctx)
    : mCtx(ctx),
      mUsage(-1),
      mShrinkCount(RECYCLER_SHRINK_COUNT),
      mShrinkDelay(RECYCLER_SHRINK_DELAY),
      mShrinkThreshold(RECYCLER_EXPAND_THRESHOLD),
      mShrinkRate(RECYCLER_SHRINK_RATE),
      mShrinkTimer(mCtx),
      bShrinking(false) {
    mControl = new detail::FControlBlock(this);
    SPDLOG_DEBUG("Create Recycler");
}

IRecyclerBase::~IRecyclerBase() {
    {
        std::unique_lock lock(mMutex);
        while (!mQueue.empty()) {
            auto *pNode = mQueue.front();
            mQueue.pop();

            pNode->DestroyElement();
            pNode->Destroy();
        }
    }

    assert(mControl != nullptr);
    mControl->Release();
    mControl->DecRefCount();

    SPDLOG_DEBUG("Destroy Recycler");
}

void IRecyclerBase::Initial(const size_t capacity) {
    if (mUsage >= 0)
        throw std::runtime_error("Recycler Has Already Initialized");

    for (size_t count = 0; count < capacity; count++) {
        auto pElem = CreateNode();
        pElem->Get()->OnCreate();

        mQueue.emplace(pElem);
    }

    mUsage = 0;
    SPDLOG_TRACE("{} - Recycler Initial", __FUNCTION__);
}

void IRecyclerBase::SetShrinkCount(const int count) {
    mShrinkCount = count;
}

void IRecyclerBase::SetShrinkDelay(const int sec) {
    mShrinkDelay = sec;
}

void IRecyclerBase::SetShrinkThreshold(const float threshold) {
    mShrinkThreshold = threshold;
}

void IRecyclerBase::SetShrinkRate(const float rate) {
    mShrinkRate = rate;
}

detail::IElementNodeBase *IRecyclerBase::AcquireNode() {
    if (mUsage < 0)
        throw std::runtime_error("Recycler Not Initialize");

    {
        std::unique_lock lock(mMutex);
        if (!mQueue.empty()) {
            auto *pResult = mQueue.front();

            mQueue.pop();
            mUsage.fetch_add(1, std::memory_order_relaxed);

            pResult->OnAcquire();
            pResult->Get()->Initial();

            SPDLOG_TRACE("{} - Recycler Acquire From Queue, Usage[{}], Idle[{}]",
                         __FUNCTION__, mUsage.load(), mQueue.size());

            return pResult;
        }
    }

    auto num = static_cast<size_t>(static_cast<float>(mUsage.load()) * RECYCLER_EXPAND_RATE);

    if (num <= 0)
        throw std::runtime_error("Recycler expand num equal zero");

    // The Last One Directly Return
    std::vector<detail::IElementNodeBase *> nodes(num - 1);
    detail::IElementNodeBase *pResult = nullptr;

    while (num-- > 0) {
        auto pElem = CreateNode();

        if (pElem == nullptr)
            continue;

        pElem->Get()->OnCreate();

        // If The Last One, As The Result
        if (num == 1) {
            pResult = pElem;
            continue;
        }

        nodes.emplace_back(pElem);
    }

    if (pResult == nullptr)
        throw std::runtime_error("Create New Element Failed");

    // size_t queueSize;

    // Emplace To The Internal Queue
    if (!nodes.empty()) {
        std::unique_lock lock(mMutex);
        for (const auto &pElem: nodes) {
            mQueue.emplace(pElem);
        }
        // queueSize = nodes.size();
    }

    mUsage.fetch_add(1, std::memory_order_relaxed);

    pResult->OnAcquire();
    pResult->Get()->Initial();

    // SPDLOG_TRACE("{} - Acquire From New, Usage[{}], Idle[{}]",
    //     __FUNCTION__, mUsage.load(), queueSize);

    return pResult;
}

void IRecyclerBase::Shrink() {
    if (mUsage < 0)
        throw std::runtime_error("Recycler Not Initialize");

    size_t num = 0;

    // Check If It Needs To Shrink
    {
        std::shared_lock lock(mMutex);

        // Recycler Total Capacity
        const size_t usage = mUsage.load();
        const size_t total = mQueue.size() + usage;

        if (total < mShrinkCount)
            return;

        // Usage Less Than Shrink Threshold
        if (static_cast<float>(usage) < std::ceil(static_cast<float>(total) * mShrinkThreshold)) {
            num = static_cast<size_t>(std::floor(static_cast<float>(total) * mShrinkRate));

            const auto rest = total - num;
            if (rest <= 0)
                return;

            if (rest < RECYCLER_MINIMUM_CAPACITY)
                num = total - RECYCLER_MINIMUM_CAPACITY;
        }
    }

    if (num <= 0)
        return;

    SPDLOG_TRACE("{:<20} - Recycler[{:p}] Need To Release {} Elements",
        __FUNCTION__, static_cast<void *>(this), num);

    std::unique_lock lock(mMutex);

    while (num-- > 0 && !mQueue.empty()) {
        auto *pNode = mQueue.front();
        mQueue.pop();

        pNode->DestroyElement();
        pNode->Destroy();
    }

    SPDLOG_TRACE("{:<20} - Recycler[{:p}] Shrink Finished",
        __FUNCTION__, static_cast<void *>(this));
}

size_t IRecyclerBase::GetUsage() const {
    const auto usage = mUsage.load(std::memory_order_acquire);
    if (usage < 0)
        return 0;
    return usage;
}

size_t IRecyclerBase::GetIdle() const {
    if (mUsage.load(std::memory_order_acquire) < 0)
        return 0;

    std::shared_lock lock(mMutex);
    return mQueue.size();
}

size_t IRecyclerBase::GetCapacity() const {
    const auto usage = mUsage.load(std::memory_order_acquire);
    if (usage < 0)
        return 0;

    std::shared_lock lock(mMutex);
    return mQueue.size() + usage;
}

void IRecyclerBase::Recycle(detail::IElementNodeBase *pNode) {
    if (mUsage < 0)
        throw std::runtime_error("Recycler Not Initialize");

    if (pNode == nullptr) [[unlikely]]
        return;

    pNode->OnRecycle();
    mUsage.fetch_sub(1, std::memory_order_relaxed);

    std::unique_lock lock(mMutex);
    mQueue.emplace(pNode);

    SPDLOG_TRACE("{} - Recycle Node, Usage[{}], Idle[{}]",
        __FUNCTION__, mUsage.load(), mQueue.size());

    if (bShrinking)
        return;

    {
        std::shared_lock localLock(mMutex);
        const auto total = mQueue.size() + mUsage;
        if (total < mShrinkCount)
            return;
    }

    bShrinking = true;
    co_spawn(mCtx, [this]() mutable -> awaitable<void> {
        try {
            mShrinkTimer.expires_after(std::chrono::seconds(mShrinkDelay));

            if (auto [ec] = co_await mShrinkTimer.async_wait(); ec)
                co_return;

            Shrink();
            bShrinking = false;

        } catch (const std::exception &e) {
            SPDLOG_ERROR("{:<20} - Recycler Exception [{}]", __FUNCTION__, e.what());
        }
    }, detached);
}
