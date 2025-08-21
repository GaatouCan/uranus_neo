#pragma once

#include "Types.h"
#include "Package.h"


class BASE_API IPackageCodec_Interface {

public:
    IPackageCodec_Interface() = default;
    virtual ~IPackageCodec_Interface() = default;

    DISABLE_COPY_MOVE(IPackageCodec_Interface)

    virtual awaitable<bool> Initial() = 0;

    virtual awaitable<bool> Encode(IPackage_Interface *pkg) = 0;
    virtual awaitable<bool> Decode(IPackage_Interface *pkg) = 0;

    virtual ATcpSocket &GetSocket() = 0;

    const ATcpSocket::executor_type &GetExecutor() {
        return GetSocket().get_executor();
    }
};


template<CPackageType Type>
class TPackageCodec : public IPackageCodec_Interface {

public:
    awaitable<bool> Encode(IPackage_Interface *pkg) override {
        if (auto temp = dynamic_cast<Type *>(pkg)) {
            const auto ret = co_await this->EncodeT(temp);
            co_return ret;
        }
        co_return false;
    }

    awaitable<bool> Decode(IPackage_Interface *pkg) override {
        if (auto temp = dynamic_cast<Type *>(pkg)) {
            const auto ret = co_await this->DecodeT(temp);
            co_return ret;
        }
        co_return false;
    }

    virtual awaitable<bool> EncodeT(Type *pkg) = 0;
    virtual awaitable<bool> DecodeT(Type *pkg) = 0;
};
