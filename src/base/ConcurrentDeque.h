#pragma once

#include <deque>
#include <condition_variable>
#include <shared_mutex>
#include <atomic>
#include <queue>

/**
 * The Thread Safe Wrapper Of std::deque
 * @tparam T Element Type
 * @tparam bNotify If True, Can Use ::Wait() And It Will Be Notified While ::PushBack(), ::PushFront() Or ::Quit()
 */
template<typename T, bool bNotify = false>
class TConcurrentDeque {

    /// The Empty Define For When It Not Need To Be Notified
    struct FEmptyConditionVariable {

    };

    using AConditionVariable = std::conditional_t<bNotify, std::condition_variable_any, FEmptyConditionVariable>;

public:
    TConcurrentDeque() = default;
    ~TConcurrentDeque() = default;

    T &Front() {
        std::shared_lock lock(mMutex);
        return mDeque.front();
    }

    T &Back() {
        std::shared_lock lock(mMutex);
        return mDeque.back();
    }

    void PushFront(const T &data) {
        {
            std::unique_lock lock(mMutex);
            mDeque.push_front(data);
        }

        if constexpr (bNotify)
            mCondVar.notify_one();
    }

    void PushBack(const T &data) {
        {
            std::unique_lock lock(mMutex);
            mDeque.push_back(data);
        }

        if constexpr (bNotify)
            mCondVar.notify_one();
    }

    void PushFront(T &&data) {
        {
            std::unique_lock lock(mMutex);
            mDeque.emplace_front(std::forward<T>(data));
        }

        if constexpr (bNotify)
            mCondVar.notify_one();
    }

    void PushBack(T &&data) {
        {
            std::unique_lock lock(mMutex);
            mDeque.emplace_back(std::forward<T>(data));
        }

        if constexpr (bNotify)
            mCondVar.notify_one();
    }

    T PopFront() {
        std::unique_lock lock(mMutex);

        auto data = std::move(mDeque.front());
        mDeque.pop_front();

        return data;
    }

    T PopBack() {
        std::unique_lock lock(mMutex);

        auto data = std::move(mDeque.back());
        mDeque.pop_back();

        return data;
    }

    bool IsEmpty() const {
        std::shared_lock lock(mMutex);
        return mDeque.empty();
    }

    size_t Size() const {
        std::shared_lock lock(mMutex);
        return mDeque.size();
    }

    void Clear() {
        std::unique_lock lock(mMutex);
        mDeque.clear();
    }

    void Quit() {
        if constexpr (bNotify) {
            bQuit = true;
            mCondVar.notify_all();
        }
    }

    bool IsRunning() const {
        if constexpr (!bNotify) {
            return false;
        }
        return !bQuit;
    }

    void Wait() {
        if constexpr (bNotify) {
            std::unique_lock lock(mMutex);
            mCondVar.wait(lock, [this] { return !mDeque.empty() || bQuit; });
        }
    }

    void SwapTo(std::deque<T> &target) {
        std::unique_lock lock(mMutex);
        target = std::move(mDeque);
    }

    void SwapTo(std::queue<T> &target) {
        std::unique_lock lock(mMutex);
        for (auto &val : mDeque) {
            target.emplace(std::forward<T>(val));
        }
        mDeque.clear();
    }

private:
    std::deque<T> mDeque;

    mutable std::shared_mutex mMutex;

    AConditionVariable mCondVar;
    std::atomic_bool bQuit{false};
};
