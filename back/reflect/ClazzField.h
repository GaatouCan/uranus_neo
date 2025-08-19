#pragma once

#include "Common.h"

#include <string>
#include <typeinfo>


class UObject;

class BASE_API IClazzField {
public:
    IClazzField() = delete;

    IClazzField(const std::string &name, size_t offset);
    virtual ~IClazzField();

    [[nodiscard]] std::string GetName() const;
    [[nodiscard]] size_t GetOffset() const;

    [[nodiscard]] virtual size_t GetSize() const = 0;
    [[nodiscard]] virtual const char * GetTypeName() const = 0;

    virtual bool SetValue(UObject *obj, void *value) const = 0;
    virtual bool GetValue(const UObject *obj, void *ret) const = 0;

    [[nodiscard]] virtual const std::type_info &GetTypeInfo() const = 0;

protected:
    const std::string mName;
    const size_t mOffset;
};

template<class Target, class Type>
class TClazzField final : public IClazzField {
public:
    TClazzField() = delete;

    TClazzField(const std::string &name, const size_t offset)
        : IClazzField(name, offset) {
    }

    [[nodiscard]] constexpr size_t GetSize() const override {
        return sizeof(Type);
    }

    [[nodiscard]] const char *GetTypeName() const override {
        return GetTypeInfo().name();
    }

    [[nodiscard]] const std::type_info &GetTypeInfo() const override {
        return typeid(std::remove_cvref_t<Type>);
    }

    bool SetValue(UObject *obj, void *value) const override;
    bool GetValue(const UObject *obj, void *ret) const override;
};

template<class Target, class Type>
inline bool TClazzField<Target, Type>::SetValue(UObject *obj, void *value) const {
    if (obj == nullptr)
        return false;

    auto target = static_cast<Target *>(obj);
    *reinterpret_cast<Type *>(reinterpret_cast<unsigned char *>(target) + mOffset) = *static_cast<Type *>(value);

    return true;
}

template<class Target, class Type>
inline bool TClazzField<Target, Type>::GetValue(const UObject *obj, void *ret) const {
    if (obj == nullptr)
        return false;

    const auto *target = static_cast<const Target *>(obj);
    auto *result = static_cast<Type *>(ret);

    *result = *reinterpret_cast<Type *>(reinterpret_cast<unsigned char *>(const_cast<Target *>(target)) + mOffset);
    return true;
}
