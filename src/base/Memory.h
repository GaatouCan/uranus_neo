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

}