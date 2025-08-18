#include "ClazzFactory.h"
#include "Clazz.h"


UClazzFactory::UClazzFactory() {
}

UClazzFactory &UClazzFactory::Instance() {
    static UClazzFactory inst;
    return inst;
}

UClazzFactory::~UClazzFactory() {
}

UClazz *UClazzFactory::FromName(const std::string &name) {
    return Instance().GetClazz(name);
}

void UClazzFactory::RegisterClazz(UClazz *clazz) {
    if (clazz == nullptr)
        return;

    mClazzMap.insert_or_assign(clazz->GetClazzName(), clazz);
}

UClazz *UClazzFactory::GetClazz(const std::string &name) const {
    const auto iter = mClazzMap.find(name);
    return iter != mClazzMap.end() ? iter->second : nullptr;
}
