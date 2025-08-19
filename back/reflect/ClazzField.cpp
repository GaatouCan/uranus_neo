#include "ClazzField.h"

IClazzField::IClazzField(const std::string &name, const size_t offset)
    : mName(name),
      mOffset(offset) {
}

IClazzField::~IClazzField() {
}

std::string IClazzField::GetName() const {
    return mName;
}

size_t IClazzField::GetOffset() const {
    return mOffset;
}
