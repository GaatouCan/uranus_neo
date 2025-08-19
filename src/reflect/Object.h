#pragma once

#include "Clazz.h"

#include <typeinfo>
#include <unordered_set>


class UGCTable;
class UObject;

class BASE_API IGCNodeBase {

    friend class UGCTable;

public:
    IGCNodeBase();
    virtual ~IGCNodeBase() = default;

    DISABLE_COPY_MOVE(IGCNodeBase)

    [[nodiscard]] virtual UObject *Get() = 0;

    void InsertSubNode(IGCNodeBase *node);

private:
    std::unordered_set<IGCNodeBase *> mFields;
    bool bMarked;
};


class BASE_API UObject {

    template<class Type>
    requires std::derived_from<Type, UObject>
    friend class TGCNode;

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

    template<class Type>
    requires std::derived_from<Type, UObject>
    Type *CreateSubObject();

protected:
    [[nodiscard]] virtual UClazz *GetClazz() const = 0;

    bool SetFieldWithTypeInfo(const std::string &name, void *value, const std::type_info &info);
    bool GetFieldWithTypeInfo(const std::string &name, void *ret, const std::type_info &info) const;

private:
    void SetUpNode(IGCNodeBase *node);

private:
    IGCNodeBase *mNode;
};

template<class Type>
requires std::derived_from<Type, UObject>
class TGCNode final : public IGCNodeBase {

public:
    TGCNode() {
        mObject.SetUpNode(this);
    }

    ~TGCNode() override {

    }

    UObject *Get() override {
        return &mObject;
    }

private:
    Type mObject;
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

template<class Type> requires std::derived_from<Type, UObject>
Type *UObject::CreateSubObject() {
    auto *node = new TGCNode<Type>();
    mNode->InsertSubNode(node);
    return node->Get();
}
