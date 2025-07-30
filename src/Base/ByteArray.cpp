#include "ByteArray.h"

#ifdef __linux__
#include <algorithm>
#endif


FByteArray::FByteArray(const size_t size)
    : mByteArray(size) {
}

FByteArray::FByteArray(const std::vector<uint8_t> &bytes)
    : mByteArray(bytes) {
}

FByteArray::operator std::vector<uint8_t>() const {
    return mByteArray;
}

void FByteArray::Reset() {
    mByteArray.clear();
    mByteArray.shrink_to_fit();
}

void FByteArray::Clear() {
    mByteArray.clear();
}

size_t FByteArray::Size() const {
    return mByteArray.size();
}

void FByteArray::Resize(const size_t size) {
    mByteArray.resize(size);
}

void FByteArray::Reserve(const size_t capacity) {
    mByteArray.reserve(capacity);
}

uint8_t *FByteArray::Data() {
    return mByteArray.data();
}

const uint8_t *FByteArray::Data() const {
    return mByteArray.data();
}

std::vector<uint8_t> &FByteArray::RawRef() {
    return mByteArray;
}

void FByteArray::FromString(const std::string_view sv) {
    mByteArray.reserve(sv.size());
    std::memcpy(mByteArray.data(), sv.data(), sv.size());
}

std::string FByteArray::ToString() const {
    return { reinterpret_cast<const char*>(mByteArray.data()), mByteArray.size() };
}

auto FByteArray::Begin() -> decltype(mByteArray)::iterator {
    return mByteArray.begin();
}

auto FByteArray::End() -> decltype(mByteArray)::iterator {
    return mByteArray.end();
}

auto FByteArray::Begin() const -> decltype(mByteArray)::const_iterator {
    return mByteArray.begin();
}

auto FByteArray::End() const -> decltype(mByteArray)::const_iterator {
    return mByteArray.end();
}

uint8_t FByteArray::operator[](const size_t pos) const noexcept {
    return mByteArray[pos];
}
