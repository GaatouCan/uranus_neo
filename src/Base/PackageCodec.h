#pragma once

#include "Package.h"
#include "Base/Types.h"


class BASE_API IPackageCodec_Interface {

public:
    IPackageCodec_Interface() = default;
    virtual ~IPackageCodec_Interface() = default;

    DISABLE_COPY_MOVE(IPackageCodec_Interface)

    virtual awaitable<bool> Initial() = 0;

    virtual awaitable<bool> Encode(const std::shared_ptr<IPackage_Interface> &pkg) = 0;
    virtual awaitable<bool> Decode(const std::shared_ptr<IPackage_Interface> &pkg) = 0;

    virtual ATcpSocket &GetSocket() = 0;
};


template<CPackageType Type>
class TPackageCodec : public IPackageCodec_Interface {

public:
    awaitable<bool> Encode(const std::shared_ptr<IPackage_Interface> &pkg) override {
        if (auto temp = std::dynamic_pointer_cast<Type>(pkg)) {
            const auto ret = co_await this->EncodeT(temp);
            co_return ret;
        }
        co_return false;
    }

    awaitable<bool> Decode(const std::shared_ptr<IPackage_Interface> &pkg) override {
        if (auto temp = std::dynamic_pointer_cast<Type>(pkg)) {
            const auto ret = co_await this->DecodeT(temp);
            co_return ret;
        }
        co_return false;
    }

    virtual awaitable<bool> EncodeT(const std::shared_ptr<Type> &pkg) = 0;
    virtual awaitable<bool> DecodeT(const std::shared_ptr<Type> &pkg) = 0;
};
