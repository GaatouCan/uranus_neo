#pragma once

#include "common.h"

#include <string>
#include <typeinfo>

class UObject;

class REFLECT_API IClazzMethod {
public:
    IClazzMethod() = delete;

    explicit IClazzMethod(std::string name);
    virtual ~IClazzMethod();

    [[nodiscard]] std::string GetName() const;

    virtual bool Invoke(UObject *obj, void *ret, void *param) const = 0;

    [[nodiscard]] virtual const std::type_info &GetReturnTypeInfo() const = 0;

private:
    const std::string mName;
};

template<class Target, class Ret, class... Args>
class TClazzMethod final : public IClazzMethod {
public:
    using MethodType = Ret (Target::*)(Args...);
    using TupleType = std::tuple<std::decay_t<Args>...>;

    TClazzMethod(std::string name, const MethodType method)
        : IClazzMethod(std::move(name)),
          mMethod(method) {
    }

    bool Invoke(UObject *obj, void *ret, void *param) const override {
        if (obj == nullptr)
            return false;

        auto *target = static_cast<Target *>(obj);
        auto *args = static_cast<TupleType *>(param);

        if constexpr (std::is_void_v<Ret>) {
            std::apply([target, this]<typename... Parameter>(Parameter &&... unpacked) -> void {
                (target->*mMethod)(std::forward<Parameter>(unpacked)...);
            }, *args);
        } else {
            auto *result = static_cast<Ret *>(ret);
            *result = std::apply([target, this]<typename... Parameter>(Parameter &&... unpacked) -> Ret {
                return (target->*mMethod)(std::forward<Parameter>(unpacked)...);
            }, *args);
        }

        return true;
    }

    [[nodiscard]] const std::type_info &GetReturnTypeInfo() const override {
        return typeid(std::remove_cvref_t<Ret>);
    }

private:
    MethodType mMethod;
};
