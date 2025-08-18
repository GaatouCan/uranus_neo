#pragma once

#include "Recycle.h"


namespace detail {
    class IElementNodeBase;
}

template<CRecycleType Type>
class FRecycleHandle final {

    friend class IRecyclerBase;
    using ElementType = std::remove_extent_t<Type>;

    explicit FRecycleHandle(detail::IElementNodeBase *pNode);

public:
    FRecycleHandle();
    FRecycleHandle(nullptr_t);
    ~FRecycleHandle();

    FRecycleHandle(const FRecycleHandle &rhs);
    FRecycleHandle &operator=(const FRecycleHandle &rhs);

    FRecycleHandle(FRecycleHandle &&rhs) noexcept;
    FRecycleHandle &operator=(FRecycleHandle &&rhs) noexcept;

    ElementType *operator->() const noexcept;
    ElementType &operator*() const noexcept;

    [[nodiscard]] ElementType *Get() const noexcept;

    template<CRecycleType T>
    [[nodiscard]] Type *GetT() const noexcept;

    [[nodiscard]] bool IsValid() const noexcept;

    void Swap(FRecycleHandle &rhs);
    void Reset() noexcept;

    template<CRecycleType T>
    FRecycleHandle<T> CastTo() const noexcept;

    bool operator==(const FRecycleHandle &rhs) const noexcept;
    bool operator==(nullptr_t) const noexcept;

private:
    template<CRecycleType T>
    FRecycleHandle(const FRecycleHandle &rhs, T *pCast);

    void Release() noexcept;

private:
    detail::IElementNodeBase *mNode;
    ElementType *mElement;
};
