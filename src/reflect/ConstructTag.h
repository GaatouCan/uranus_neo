#pragma once

#include <string>
#include <vector>


template<class T>
std::string GetTypeName() {
    using Type = std::remove_cv_t<std::remove_reference_t<T> >;
    return typeid(Type).name();
}

template<typename... Args>
struct FConstructorTag {
    static std::vector<std::string> GetArgumentNames() {
        return {GetTypeName<Args>()...};
    }

    static size_t GetArgumentCount() {
        return sizeof...(Args);
    }
};
