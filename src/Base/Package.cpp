#include "Package.h"

#ifdef __linux__
#include <cstring>
#endif


inline constexpr int PACKET_MAGIC = 20250514;

FPackage::FPackage()
    : mHeader() {
    memset(&mHeader, 0, sizeof(mHeader));
}

FPackage::~FPackage() = default;

bool FPackage::IsUnused() const {
    // As Same As Assigned In Initial() Method
    return mHeader.id == (MINIMUM_PACKAGE_ID - 1);
}

void FPackage::OnCreate() {
    SetMagic(PACKET_MAGIC);
}

void FPackage::Initial() {
    mHeader.id = MINIMUM_PACKAGE_ID - 1;
    mHeader.source = -1;
    mHeader.target = -1;
}

void FPackage::Clear() {
    mHeader.id = 0;
    mPayload.Clear();
}

bool FPackage::CopyFrom(IRecycle_Interface *other) {
    if (IRecycle_Interface::CopyFrom(other)) {
        if (const auto temp = dynamic_cast<FPackage *>(other); temp != nullptr) {
            memcpy(&mHeader, &temp->mHeader, sizeof(mHeader));

            mPayload = temp->mPayload;
            mHeader.length = temp->mHeader.length;

            return true;
        }
    }
    return false;
}

bool FPackage::CopyFrom(const std::shared_ptr<IRecycle_Interface> &other) {
    if (IRecycle_Interface::CopyFrom(other)) {
        if (const auto temp = std::dynamic_pointer_cast<FPackage>(other); temp != nullptr) {
            memcpy(&mHeader, &temp->mHeader, sizeof(mHeader));

            mPayload = temp->mPayload;
            mHeader.length = temp->mHeader.length;

            return true;
        }
    }
    return false;
}

void FPackage::Reset() {
    memset(&mHeader, 0, sizeof(mHeader));
    mPayload.Reset();
}

bool FPackage::IsAvailable() const {
    if (IsUnused())
        return true;

    return mHeader.id >= MINIMUM_PACKAGE_ID && mHeader.id <= MAXIMUM_PACKAGE_ID;
}

void FPackage::SetPackageID(const uint32_t id) {
    mHeader.id = id;
}

FPackage &FPackage::SetData(const std::string_view str) {
    mHeader.length = str.size();
    mPayload.FromString(str);
    return *this;
}

FPackage &FPackage::SetData(const std::stringstream &ss) {
    return SetData(ss.str());
}

FPackage &FPackage::SetMagic(const uint32_t magic) {
    mHeader.magic = magic;
    return *this;
}

uint32_t FPackage::GetMagic() const {
    return mHeader.magic;
}

uint32_t FPackage::GetPackageID() const {
    return mHeader.id;
}

size_t FPackage::GetPayloadLength() const {
    return mPayload.Size();
}

void FPackage::SetSource(const int32_t source) {
    mHeader.source = source;
}

int32_t FPackage::GetSource() const {
    return mHeader.source;
}

void FPackage::SetTarget(const int32_t target) {
    mHeader.target = target;
}

int32_t FPackage::GetTarget() const {
    return mHeader.target;
}

std::string FPackage::ToString() const {
    return mPayload.ToString();
}

const FByteArray &FPackage::RawPayload() const {
    return mPayload;
}

std::vector<uint8_t> &FPackage::RawRef() {
    return mPayload.RawRef();
}
