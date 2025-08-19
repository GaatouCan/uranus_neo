#pragma once

#include "Common.h"
#include "ConstructTag.h"

#include <typeindex>


class UObject;


class BASE_API IConstructor {
public:
    IConstructor() = default;
    virtual ~IConstructor() = default;

    [[nodiscard]] virtual std::type_index GetTypeIndex() const = 0;

    [[nodiscard]] virtual size_t GetArgumentCount() const = 0;
    [[nodiscard]] virtual std::vector<std::string> GetArgumentList() const = 0;

    [[nodiscard]] virtual UObject *Create(void *param) const = 0;

    template<class... Args>
    UObject *CreateInstance(Args &&... args) const {
        if (sizeof...(Args) != this->GetArgumentCount()) {
            return nullptr;
        }

        using TupleType = std::tuple<std::decay_t<Args>...>;
        TupleType param = std::make_tuple(std::forward<Args>(args)...);
        return this->Create(reinterpret_cast<void *>(&param));
    }
};


template<class Target, typename... Args>
class TConstructor final : public IConstructor {

public:
    using TagType = FConstructorTag<std::decay_t<Args>...>;
    using TupleType = std::tuple<std::decay_t<Args>...>;

    [[nodiscard]] std::type_index GetTypeIndex() const override {
        return typeid(TagType);
    }

    [[nodiscard]] size_t GetArgumentCount() const override {
        return TagType::GetArgumentCount();
    }

    [[nodiscard]] std::vector<std::string> GetArgumentList() const override {
        return TagType::GetArgumentNames();
    }

    [[nodiscard]] UObject *Create(void *ptr) const override {
        auto *args = static_cast<TupleType *>(ptr);

        return std::apply([this]<typename... Parameter>(Parameter &&... param) {
            return new Target(std::forward<Parameter>(param)...);
        }, *args);
    }
};
