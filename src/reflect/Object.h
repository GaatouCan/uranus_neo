#pragma once

#include <typeinfo>

#include "Clazz.h"


class BASE_API UObject {

public:
    UObject();
    virtual ~UObject();

    bool SetFieldValue(const std::string &name, void *value);
    bool GetFieldValue(const std::string &name, void *ret) const;

    bool InvokeMethod(const std::string& name, void *ret, void *param);

    template<class Type>
    bool SetField(const std::string &name, Type && value);

    template<class Type>
    std::tuple<Type, bool> GetField(const std::string &name) const;

    template<class ... Args>
    bool Invoke(const std::string& name, void *ret, Args&& ... args);

protected:
    [[nodiscard]] virtual UClazz *GetClazz() const = 0;

    bool SetFieldWithTypeInfo(const std::string &name, void *value, const std::type_info &info);
    bool GetFieldWithTypeInfo(const std::string &name, void *ret, const std::type_info &info) const;
};


template<class Type>
bool UObject::SetField(const std::string &name, Type &&value) {
    return this->SetFieldWithTypeInfo(name, static_cast<void*>(const_cast<std::remove_cv_t<Type>*>(&value)), typeid(std::remove_cvref_t<Type>));
}

template<class Type>
std::tuple<Type, bool> UObject::GetField(const std::string &name) const {
    Type result;
    if (this->GetFieldWithTypeInfo(name, &result, typeid(std::remove_cvref_t<Type>)))
        return { result, true };
    return { result, false };
}

template<class ... Args>
inline bool UObject::Invoke(const std::string &name, void *ret, Args &&...args) {
    using TupleType = std::tuple<std::decay_t<Args>...>;
    TupleType param = std::make_tuple(std::forward<Args>(args)...);
    return this->InvokeMethod(name, ret, reinterpret_cast<void *>(&param));
}
