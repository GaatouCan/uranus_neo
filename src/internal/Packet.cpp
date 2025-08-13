#include "Packet.h"

#ifdef __linux__
#include <cstring>
#endif


inline constexpr int PACKET_MAGIC = 20250514;

FPacket::FPacket()
    : mHeader() {
    memset(&mHeader, 0, sizeof(mHeader));
}

FPacket::~FPacket() = default;

bool FPacket::IsUnused() const {
    // As Same As Assigned In Initial() Method
    return mHeader.id == (MINIMUM_PACKAGE_ID - 1);
}

void FPacket::OnCreate() {
    SetMagic(PACKET_MAGIC);
}

void FPacket::Initial() {
    mHeader.id = MINIMUM_PACKAGE_ID - 1;
    mHeader.source = -1;
    mHeader.target = -1;
}

void FPacket::Clear() {
    mHeader.id = 0;
    mPayload.Clear();
}

bool FPacket::CopyFrom(IRecycle_Interface *other) {
    if (IRecycle_Interface::CopyFrom(other)) {
        if (const auto temp = dynamic_cast<FPacket *>(other); temp != nullptr) {
            memcpy(&mHeader, &temp->mHeader, sizeof(mHeader));

            mPayload = temp->mPayload;
            mHeader.length = temp->mHeader.length;

            return true;
        }
    }
    return false;
}

// bool FPacket::CopyFrom(const std::shared_ptr<IRecycle_Interface> &other) {
//     if (IRecycle_Interface::CopyFrom(other)) {
//         if (const auto temp = std::dynamic_pointer_cast<FPacket>(other); temp != nullptr) {
//             memcpy(&mHeader, &temp->mHeader, sizeof(mHeader));
//
//             mPayload = temp->mPayload;
//             mHeader.length = temp->mHeader.length;
//
//             return true;
//         }
//     }
//     return false;
// }

void FPacket::Reset() {
    memset(&mHeader, 0, sizeof(mHeader));
    mPayload.Reset();
}

bool FPacket::IsAvailable() const {
    if (IsUnused())
        return true;

    return mHeader.id >= MINIMUM_PACKAGE_ID && mHeader.id <= MAXIMUM_PACKAGE_ID;
}

void FPacket::SetPackageID(const uint32_t id) {
    mHeader.id = id;
}

FPacket &FPacket::SetData(const std::string_view str) {
    mHeader.length = str.size();
    mPayload.FromString(str);
    return *this;
}

FPacket &FPacket::SetData(const std::stringstream &ss) {
    return SetData(ss.str());
}

FPacket &FPacket::SetMagic(const uint32_t magic) {
    mHeader.magic = magic;
    return *this;
}

uint32_t FPacket::GetMagic() const {
    return mHeader.magic;
}

uint32_t FPacket::GetPackageID() const {
    return mHeader.id;
}

size_t FPacket::GetPayloadLength() const {
    return mPayload.Size();
}

void FPacket::SetSource(const int32_t source) {
    mHeader.source = source;
}

int32_t FPacket::GetSource() const {
    return mHeader.source;
}

void FPacket::SetTarget(const int32_t target) {
    mHeader.target = target;
}

int32_t FPacket::GetTarget() const {
    return mHeader.target;
}

std::string FPacket::ToString() const {
    return mPayload.ToString();
}

const FByteArray &FPacket::RawPayload() const {
    return mPayload;
}

std::vector<uint8_t> &FPacket::RawRef() {
    return mPayload.RawRef();
}
