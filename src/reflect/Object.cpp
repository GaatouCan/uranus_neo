#include "Object.h"
#include "ClazzMethod.h"
#include "ClazzField.h"

IGCNodeBase::IGCNodeBase()
    : bMarked(false) {
}

void IGCNodeBase::InsertSubNode(IGCNodeBase *node) {
    mFields.insert(node);
}

UObject::UObject()
    : mNode(nullptr) {
}

UObject::~UObject() {
}

bool UObject::SetFieldValue(const std::string &name, void *value) {
    const auto *clazz = GetClazz();
    if (!clazz)
        return false;

    const auto *field = clazz->FindField(name);
    if (!field)
        return field;

    return field->SetValue(this, value);
}

bool UObject::GetFieldValue(const std::string &name, void *ret) const {
    const auto *clazz = GetClazz();
    if (!clazz)
        return false;

    const auto *field = clazz->FindField(name);
    if (!field)
        return false;

    return field->GetValue(this, ret);
}

bool UObject::InvokeMethod(const std::string &name, void *ret, void *param) {
    const auto *clazz = GetClazz();
    if (!clazz)
        return false;

    const auto *method = clazz->FindMethod(name);
    if (!method)
        return false;

    return method->Invoke(this, ret, param);
}

bool UObject::SetFieldWithTypeInfo(const std::string &name, void *value, const std::type_info &info) {
    const auto *clazz = GetClazz();
    if (!clazz)
        return false;

    const auto *field = clazz->FindField(name);
    if (!field)
        return false;

    if (field->GetTypeInfo() != info)
        return false;

    return field->SetValue(this, value);
}

bool UObject::GetFieldWithTypeInfo(const std::string &name, void *ret, const std::type_info &info) const {
    const auto *clazz = GetClazz();
    if (!clazz)
        return false;

    const auto *field = clazz->FindField(name);
    if (!field)
        return false;

    if (field->GetTypeInfo() != info)
        return false;

    return field->GetValue(this, ret);
}

void UObject::SetUpNode(IGCNodeBase *node) {
    mNode = node;
}
