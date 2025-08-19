#include "ClazzMethod.h"

#include <utility>

IClazzMethod::IClazzMethod(std::string name)
    : mName(std::move(name)) {
}

IClazzMethod::~IClazzMethod() {
}

std::string IClazzMethod::GetName() const {
    return mName;
}
