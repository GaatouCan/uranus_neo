#pragma once

#include "Common.h"

#include <atomic>
#include <typeinfo>

#ifdef _MSC_VER
#define NOVTABLE __declspec(novtable)
#else
#define NOVTABLE
#endif

namespace memory {

    class BASE_API IRefCountBase {

    public:
        DISABLE_COPY(IRefCountBase)

        virtual ~IRefCountBase() noexcept = default;

        bool IncRefCount_NotZero();

        virtual void IncRefCount();
        virtual void DecRefCount();

        [[nodiscard]] size_t GetRefCount() const noexcept;
        virtual void *GetDeleter(const type_info &) const noexcept { return nullptr; }

    protected:
        constexpr IRefCountBase() noexcept
            : mRefCount(1) {}

        virtual void Destroy() noexcept = 0;
        virtual void Delete() noexcept = 0;

    protected:
        std::atomic_size_t mRefCount;
    };

    class BASE_API ISharedCountBase : public IRefCountBase {

    protected:
        constexpr ISharedCountBase() noexcept
            : mWeakCount(1) {

        }

    public:
        DISABLE_COPY(ISharedCountBase)

        void DecRefCount() override;

        void IncWeakCount();
        void DecWeakCount();

    protected:
        std::atomic_size_t mWeakCount;
    };

    template<class T>
    class TSharedRefCount final : public ISharedCountBase {

        using ElementType = typename std::remove_cv_t<T>;

    public:
        explicit TSharedRefCount(ElementType *element) noexcept
            : mPtr(element) {

        }

    protected:
        void Destroy() noexcept override {
            delete mPtr;
        }

        void Delete() noexcept override {
            delete this;
        }

    private:
        ElementType *mPtr;
    };

    template<class T>
    class TSharedPointer;

    template<class T>
    class TWeakPointer;

    template<class T>
    class ISharedBase {
    public:
        using ElementType = typename std::remove_cv_t<T>;

        [[nodiscard]] size_t GetUseCount() const noexcept {
            return mRef ? mRef->GetRefCount() : 0;
        }

        template<class T>
        [[nodiscard]] bool IsOwnerBefore(const ISharedBase<T> &other) const noexcept {
            return mRef < other.mRef;
        }

        DISABLE_COPY(ISharedBase)

    protected:
        [[nodiscard]] ElementType *Get() const noexcept {
            return mPtr;
        }

        constexpr ISharedBase() noexcept = default;
        virtual ~ISharedBase() noexcept = default;

        template<typename Ty>
        void MoveFrom(ISharedBase<Ty> &&other) {
            mPtr = other.mPtr;
            mRef = other.mRef;

            other.mPtr = nullptr;
            other.mRef = nullptr;
        }

        template<typename Ty>
        void CopyFrom(const TSharedPointer<Ty> &other) noexcept {
            other.IncRefCount();

            mPtr = other.mPtr;
            mRef = other.mRef;
        }

        template<typename Ty>
        void CastFrom(const TSharedPointer<Ty> &other, ElementType *ptr) noexcept {
            other.IncRefCount();

            mPtr = ptr;
            mRef = other.mRef;
        }

        template<typename Ty>
        void MoveCastFrom(TSharedPointer<Ty> &&other, ElementType *ptr) noexcept {
            mPtr = ptr;
            mRef = other.mRef;

            other.mPtr = nullptr;
            other.mRef = nullptr;
        }

        void IncRefCount()  const noexcept { if (mRef) mRef->IncRefCount();     }
        void DecRefCount()  const noexcept { if (mRef) mRef->DecRefCount();     }
        void IncWeakCount() const noexcept { if (mRef) mRef->IncWeakCount();    }
        void DecWeakCount() const noexcept { if (mRef) mRef->DecWeakCount();    }

    private:
        ElementType *       mPtr{ nullptr };
        ISharedCountBase *  mRef{ nullptr };
    };

    template<class T>
    class TSharedPointer : public ISharedBase<T> {

    };

    template<class T>
    class TWeakPointer : public ISharedBase<T> {

    };

}