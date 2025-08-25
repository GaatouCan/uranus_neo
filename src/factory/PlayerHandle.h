#pragma once

#include "Common.h"

#include <cstddef>


class IPlayerBase;
class IPlayerFactory_Interface;


class BASE_API FPlayerHandle final {

    IPlayerBase *mPlayer;
    IPlayerFactory_Interface *mFactory;

public:
    FPlayerHandle();

    FPlayerHandle(IPlayerBase *pPlayer, IPlayerFactory_Interface *pFactory);
    ~FPlayerHandle();

    DISABLE_COPY(FPlayerHandle)

    FPlayerHandle(FPlayerHandle &&rhs) noexcept;
    FPlayerHandle &operator=(FPlayerHandle &&rhs) noexcept;

    [[nodiscard]] bool IsValid() const;

    IPlayerBase *operator->() const noexcept;
    IPlayerBase &operator*() const noexcept;

    [[nodiscard]] IPlayerBase *Get() const noexcept;

    bool operator==(const FPlayerHandle &rhs) const noexcept;
    bool operator==(nullptr_t) const noexcept;

    explicit operator bool() const noexcept;

};
