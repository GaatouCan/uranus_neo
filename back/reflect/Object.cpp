#include "Object.h"
#include "ClazzMethod.h"
#include "ClazzField.h"


void detail::FObjectControl::Erase(UObject *obj) {
    nodes.erase(obj);
}

void detail::FObjectControl::Insert(UObject *obj) {
    nodes.insert(obj);
}

bool detail::FObjectControl::IsMarked() const {
    return bMarked;
}

void detail::FObjectControl::Mark() {
    bMarked = true;
}

void detail::FObjectControl::ResetMark() {
    bMarked = false;
}

UObject::UObject() {
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

void UObject::AttachTo(UObject *obj) {
    if (!obj)
        return;

    mControl.parent->mControl.Erase(this);
    obj->mControl.Insert(this);
    mControl.parent = obj;
}

void UObject::SetUpParent(UObject *parent) {
    mControl.parent = parent;
}
