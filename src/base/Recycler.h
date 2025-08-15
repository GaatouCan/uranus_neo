#pragma once

#include "Common.h"
#include "Recycle.h"

#include <queue>
#include <atomic>
#include <shared_mutex>
#include <memory>

namespace recycle {
    namespace detail {

#pragma region Recycler Control Block
        class BASE_API FControlBlock {

            std::atomic_int64_t             mRefCount;
            std::atomic<IRecyclerBase *>    mRecycler;

        public:
            FControlBlock() = delete;

            explicit FControlBlock(IRecyclerBase *pRecycler);
            ~FControlBlock();

            DISABLE_COPY_MOVE(FControlBlock)

            void Release();

            [[nodiscard]] int64_t           GetRefCount()   const;
            [[nodiscard]] bool              IsValid()       const;
            [[nodiscard]] IRecyclerBase *   Get()           const;

            void IncRefCount();
            void DecRefCount();
        };
#pragma endregion

#pragma region Recycle Element Control Node

        inline constexpr int64_t RECYCLED_REFERENCE_COUNT = -10;

        class BASE_API IElementNodeBase {
        public:
            IElementNodeBase() = delete;

            explicit IElementNodeBase(FControlBlock *pCtrl);
            virtual ~IElementNodeBase();

            DISABLE_COPY_MOVE(IElementNodeBase)

            void OnAcquire();
            void OnRecycle();

            void IncRefCount();
            bool DecRefCount();

            virtual void DestroyElement() noexcept = 0;
            virtual void Destroy() noexcept = 0;

            [[nodiscard]] virtual       IRecycle_Interface *    Get()       noexcept = 0;
            [[nodiscard]] virtual const IRecycle_Interface *    Get() const noexcept = 0;

            [[nodiscard]] IRecyclerBase *GetRecycler() const noexcept;

        private:
            std::atomic_int64_t     mRefCount;
            FControlBlock * const   mControl;
        };
    }
#pragma endregion

    class BASE_API FRecycleHandle final {

        friend class IRecyclerBase;

        explicit FRecycleHandle(detail::IElementNodeBase *pNode);
    public:
        FRecycleHandle() = delete;
        ~FRecycleHandle();

        FRecycleHandle(const FRecycleHandle &rhs);
        FRecycleHandle &operator=(const FRecycleHandle &rhs);
        FRecycleHandle(FRecycleHandle &&rhs) noexcept;
        FRecycleHandle &operator=(FRecycleHandle &&rhs) noexcept;

        IRecycle_Interface *operator->  () const noexcept;
        IRecycle_Interface &operator*   () const noexcept;

        [[nodiscard]] IRecycle_Interface *Get() const noexcept;

    private:
        void Release() noexcept;

    private:
        detail::IElementNodeBase *  mNode;
        IRecycle_Interface *        mElement;
    };

    class BASE_API IRecyclerBase {

        friend class FRecycleHandle;

        static constexpr float      RECYCLER_EXPAND_THRESHOLD   = 0.75f;
        static constexpr float      RECYCLER_EXPAND_RATE        = 1.f;

        static constexpr int        RECYCLER_SHRINK_DELAY       = 1;
        static constexpr float      RECYCLER_SHRINK_THRESHOLD   = 0.3f;
        static constexpr float      RECYCLER_SHRINK_RATE        = 0.5f;
        static constexpr int        RECYCLER_MINIMUM_CAPACITY   = 64;

    protected:
        IRecyclerBase();

        virtual detail::IElementNodeBase *CreateNode() const = 0;

    public:
        virtual ~IRecyclerBase();

        DISABLE_COPY_MOVE(IRecyclerBase)

        void Initial(size_t capacity = 64);

        FRecycleHandle Acquire();
        void Shrink();

        [[nodiscard]] size_t GetUsage()     const;
        [[nodiscard]] size_t GetIdle()      const;
        [[nodiscard]] size_t GetCapacity()  const;

        template<
            CRecycleType Type,
            class Allocator             = std::allocator<Type>,
            class Deleter               = std::default_delete<Type>>
        static IRecyclerBase *Create();

    private:
        void Recycle(detail::IElementNodeBase *pNode);

    private:
        std::queue<detail::IElementNodeBase *>  mQueue;
        mutable std::shared_mutex               mMutex;
        std::atomic_int64_t                     mUsage;

    protected:
        detail::FControlBlock *mControl;
    };

    namespace detail {

        template<class Alloc, class Other>
        using RebindAlloc = std::allocator_traits<Alloc>::template rebind_alloc<Other>;

        template<class Type, class Alloc>
        constexpr bool CheckStandardAllocator =
            std::is_same_v<RebindAlloc<Alloc, Type>, std::allocator<Type>>;

        template<class Type, class Deleter>
        constexpr bool CheckDefaultDeleter =
            std::is_same_v<std::remove_cv_t<std::remove_reference_t<Deleter>>, std::default_delete<Type>>;

        template<CRecycleType Type, typename Allocator>
        class FElementNodeInplace final : public IElementNodeBase {

            using Traits        = std::allocator_traits<Allocator>;
            using AllocNode     = RebindAlloc<Allocator, FElementNodeInplace>;

        public:
            explicit FElementNodeInplace(FControlBlock *pControl, const AllocNode &alloc)
                : IElementNodeBase(pControl),
                  mAllocator(alloc),
                  mElement{} {
                ::new (static_cast<void*>(&mElement)) Type();
            }

            Type *GetT() noexcept {
                return std::launder(reinterpret_cast<Type*>(&mElement));
            }

            const Type *GetT() const noexcept {
                return std::launder(reinterpret_cast<const Type*>(&mElement));
            }

            [[nodiscard]] IRecycle_Interface *Get() noexcept override {
                return static_cast<IRecycle_Interface *>(GetT());
            }

            [[nodiscard]] const IRecycle_Interface *Get() const noexcept override {
                return static_cast<const IRecycle_Interface *>(GetT());
            }

            void DestroyElement() noexcept override {
                (*GetT()).~Type();
            }

            void Destroy() noexcept override {
                AllocNode alloc(mAllocator);
                auto *self = this;
                std::allocator_traits<AllocNode>::destroy(alloc, self);
                std::allocator_traits<AllocNode>::deallocate(alloc, self, 1);
            }

        private:
            Allocator mAllocator;
            alignas(Type) std::byte mElement[sizeof(Type)];
        };

        template<CRecycleType Type, typename Allocator, typename Deleter>
        class FElementNodeSeparate final : public IElementNodeBase {
            using Traits        = std::allocator_traits<Allocator>;
            using AllocType     = RebindAlloc<Allocator, Type>;
            using AllocNode     = RebindAlloc<Allocator, FElementNodeSeparate>;

        public:
            FElementNodeSeparate(FControlBlock *pControl, const AllocNode &alloc, const Deleter &deleter)
                : IElementNodeBase(pControl),
                  mAllocator(alloc),
                  mDeleter(deleter) {
                AllocType allocType(mAllocator);
                mElement = Traits::allocate(allocType, 1);
                try {
                    Traits::construct(allocType, mElement);
                } catch (...) {
                    Traits::deallocate(allocType, mElement, 1);
                    throw;
                }
            }

            void DestroyElement() noexcept override {
                if constexpr (!CheckDefaultDeleter<Type, Deleter>) {
                    std::invoke(mDeleter, mElement);
                } else {
                    AllocType allocType(mAllocator);
                    std::allocator_traits<AllocType>::destroy(allocType, mElement);
                    std::allocator_traits<AllocType>::deallocate(allocType, mElement, 1);
                }
                mElement = nullptr;
            }

            void Destroy() noexcept override {
                AllocNode allocNode(mAllocator);
                auto *self = this;
                std::allocator_traits<AllocNode>::destroy(allocNode, self);
                std::allocator_traits<AllocNode>::deallocate(allocNode, self, 1);
            }

            [[nodiscard]] IRecycle_Interface *Get() noexcept override {
                return static_cast<IRecycle_Interface *>(mElement);
            }

            [[nodiscard]] const IRecycle_Interface *Get() const noexcept override {
                return static_cast<const IRecycle_Interface *>(mElement);
            }

        private:
            AllocType   mAllocator;
            Deleter     mDeleter;
            Type *      mElement;
        };

        /// Create Node By Check If It Uses Default Allocator And Deleter Or Not
        template<CRecycleType Type, class Allocator, class Deleter>
        IElementNodeBase *CreateElementNode(FControlBlock *pCtrl, const Allocator &alloc, const Deleter &deleter) {
            if constexpr (CheckStandardAllocator<Type, Allocator> && CheckDefaultDeleter<Type, Deleter>) {
                using Node      = FElementNodeInplace<Type, Allocator>;
                using AllocNode = RebindAlloc<Allocator, Node>;
                AllocNode allocNode(alloc);
                Node* res = std::allocator_traits<AllocNode>::allocate(allocNode, 1);
                try {
                    std::allocator_traits<AllocNode>::construct(allocNode, res, pCtrl, alloc);
                } catch (...) {
                    std::allocator_traits<AllocNode>::deallocate(allocNode, res, 1);
                    throw;
                }
                return res;
            } else {
                using Node      = FElementNodeSeparate<Type, Allocator, Deleter>;
                using AllocNode = RebindAlloc<Allocator, Node>;

                AllocNode allocNode(alloc);
                Node* res = std::allocator_traits<AllocNode>::allocate(allocNode, 1);
                try {
                    std::allocator_traits<AllocNode>::construct(allocNode, res, pCtrl, alloc, deleter);
                } catch (...) {
                    std::allocator_traits<AllocNode>::deallocate(allocNode, res, 1);
                    throw;
                }
                return res;
            }
        }
    }

    template<
        CRecycleType Type,
        class Allocator     = std::allocator<Type>,
        class Deleter       = std::default_delete<Type>>
    class TRecycler final : public IRecyclerBase {
    protected:
        detail::IElementNodeBase *CreateNode() const override {
            return detail::CreateElementNode<Type, Allocator, Deleter>(mControl, mAllocator, mDeleter);
        }

    private:
        Allocator   mAllocator{};
        Deleter     mDeleter{};
    };

    template<CRecycleType Type, class Allocator, class Deleter>
    inline IRecyclerBase *IRecyclerBase::Create() {
        return new TRecycler<Type, Allocator, Deleter>();
    }
}

using recycle::IRecyclerBase;
using recycle::FRecycleHandle;
