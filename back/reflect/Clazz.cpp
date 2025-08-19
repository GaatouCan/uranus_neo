#include "Clazz.h"
#include "ClazzField.h"
#include "ClazzMethod.h"

#include <ranges>
#include <algorithm>

UClazz::UClazz()
    : mParentClazz(nullptr) {
}

UClazz::~UClazz() {
    for (const auto &val: mConstructorMap | std::views::values) {
        delete val;
    }

    for (const auto &field: mFieldMap | std::views::values) {
        delete field;
    }

    for (const auto &method: mMethodMap | std::views::values) {
        delete method;
    }
}

IConstructor *UClazz::FindConstructor(const std::type_index &type) const {
    const auto iter = mConstructorMap.find(type);
    return iter != mConstructorMap.end() ? iter->second : nullptr;
}

IClazzField *UClazz::FindField(const std::string &name) const {
    const auto iter = mFieldMap.find(name);
    return iter != mFieldMap.end() ? iter->second : nullptr;
}

IClazzMethod *UClazz::FindMethod(const std::string &name) const {
    const auto iter = mMethodMap.find(name);
    return iter != mMethodMap.end() ? iter->second : nullptr;
}

void UClazz::ForeachFieldByOrder(const std::function<bool(const IClazzField *)> &func) const {
    for (const auto &val: mOrderedField) {
        if (func(val))
            return;
    }
}

std::vector<IClazzField *> UClazz::GetOrderedFields() const {
    return mOrderedField;
}

UClazz *UClazz::GetParentClazz() const {
    return mParentClazz;
}

void UClazz::SetUpParentClazz(UClazz *parent) {
    if (parent) {
        mParentClazz = parent;
    }
}

void UClazz::RegisterConstructor(IConstructor *constructor) {
    if (constructor == nullptr) return;
    mConstructorMap.insert_or_assign(constructor->GetTypeIndex(), constructor);
}

void UClazz::RegisterField(IClazzField *field) {
    if (field == nullptr) return;
    mFieldMap.insert_or_assign(field->GetName(), field);
}

void UClazz::RegisterMethod(IClazzMethod *method) {
    if (method == nullptr) return;
    mMethodMap.insert_or_assign(method->GetName(), method);
}

void UClazz::SortFields() {
    mOrderedField.clear();
    for (const auto &field: mFieldMap | std::views::values) {
        mOrderedField.emplace_back(field);
    }

    std::ranges::sort(mOrderedField, [](const IClazzField *lhs, const IClazzField *rhs) {
        return lhs->GetOffset() < rhs->GetOffset();
    });
}
