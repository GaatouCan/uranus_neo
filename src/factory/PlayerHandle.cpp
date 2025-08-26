#include "PlayerHandle.h"
#include "gateway/PlayerBase.h"
#include "PlayerFactory.h"


FPlayerHandle::FPlayerHandle()
    : mPlayer(nullptr),
      mFactory(nullptr) {
}

FPlayerHandle::FPlayerHandle(IPlayerBase *pPlayer, IPlayerFactory_Interface *pFactory)
    : mPlayer(pPlayer),
      mFactory(pFactory) {
}

FPlayerHandle::~FPlayerHandle() {
    Release();
}

FPlayerHandle::FPlayerHandle(FPlayerHandle &&rhs) noexcept {
    mPlayer = rhs.mPlayer;
    mFactory = rhs.mFactory;
    rhs.mPlayer = nullptr;
    rhs.mFactory = nullptr;
}

FPlayerHandle &FPlayerHandle::operator=(FPlayerHandle &&rhs) noexcept {
    if (this != &rhs) {
        Release();

        mPlayer = rhs.mPlayer;
        mFactory = rhs.mFactory;
        rhs.mPlayer = nullptr;
        rhs.mFactory = nullptr;
    }
    return *this;
}

bool FPlayerHandle::IsValid() const {
    return mPlayer != nullptr;
}

IPlayerBase *FPlayerHandle::operator->() const noexcept {
    return mPlayer;
}

IPlayerBase &FPlayerHandle::operator*() const noexcept {
    return *mPlayer;
}

IPlayerBase *FPlayerHandle::Get() const noexcept {
    return mPlayer;
}

bool FPlayerHandle::operator==(const FPlayerHandle &rhs) const noexcept {
    return mPlayer == rhs.mPlayer && mFactory == rhs.mFactory;
}

bool FPlayerHandle::operator==(nullptr_t) const noexcept {
    return mPlayer == nullptr;
}

FPlayerHandle::operator bool() const noexcept {
    return IsValid();
}

void FPlayerHandle::Release() noexcept {
    if (mPlayer == nullptr)
        return;

    if (mFactory != nullptr) {
        mFactory->DestroyPlayer(mPlayer);
    } else {
        delete mPlayer;
    }

    mPlayer = nullptr;
    mFactory = nullptr;
}
