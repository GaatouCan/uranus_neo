#pragma once

#include "common.h"
#include "Constructor.h"

#include <unordered_map>
#include <typeindex>
#include <functional>

#include "ClazzField.h"

class UObject;
class IClazzField;
class IClazzMethod;
class IConstructor;
class UClazzFactory;

class REFLECT_API UClazz {

protected:
    UClazz();

public:
    virtual ~UClazz();

    DISABLE_COPY_MOVE(UClazz)

    [[nodiscard]] virtual const char *GetClazzName() const = 0;

    // [[nodiscard]] virtual UObject *Create() const = 0;
    virtual void Destroy(UObject *obj) const = 0;

    [[nodiscard]] virtual size_t GetClazzSize() const = 0;

    [[nodiscard]] IConstructor *FindConstructor(const std::type_index &type) const;

    [[nodiscard]] IClazzField *FindField(const std::string &name) const;
    [[nodiscard]] IClazzMethod *FindMethod(const std::string &name) const;

    void ForeachFieldByOrder(const std::function<bool(const IClazzField *)> &func) const;
    [[nodiscard]] std::vector<IClazzField *> GetOrderedFields() const;

    template<class ... Args>
    UObject *CreateObject(Args && ... args) const;

    [[nodiscard]] UClazz *GetParentClazz() const;

protected:
    void SetUpParentClazz(UClazz *parent);

    void RegisterConstructor(IConstructor *constructor);
    void RegisterField(IClazzField *field);
    void RegisterMethod(IClazzMethod *method);

    void SortFields();

protected:
    UClazz *mParentClazz;

    std::unordered_map<std::type_index, IConstructor *> mConstructorMap;
    std::unordered_map<std::string, IClazzField *> mFieldMap;
    std::unordered_map<std::string, IClazzMethod *> mMethodMap;

    /** Fields Sorted By Less Offset **/
    std::vector<IClazzField *> mOrderedField;
};

template<class ... Args>
inline UObject *UClazz::CreateObject(Args &&...args) const {
    using TagType = FConstructorTag<std::decay_t<Args>...>;

    const auto *constructor = FindConstructor(typeid(TagType));
    if (constructor == nullptr)
        return nullptr;

    return constructor->CreateInstance(std::forward<Args>(args)...);
}
