#include "AppearComponent.h"
#include "../../Protocol.h"

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/array.hpp>

void protocol::AppearanceRequest(uint32_t pid, const std::shared_ptr<FPacket> &pkg, UPlayer *plr) {
}

UAppearComponent::UAppearComponent()
    : mCurrentIndex(0) {

}

UAppearComponent::~UAppearComponent() {
}

DB_SERIALIZE_BEGIN(UAppearComponent)
    DB_WRITE_FIELD("current_index", mCurrentIndex);
    DB_WRITE_ARRAY("avatar_list", mAvatarList,
        DB_WRITE_ARRAY_ELEMENT("index", index),
        DB_WRITE_ARRAY_ELEMENT("active", active))
DB_SERIALIZE_END

DB_DESERIALIZE_BEGIN(UAppearComponent)
    DB_READ_FIELD("current_index", int32, mCurrentIndex)
    DB_READ_ARRAY("avatar_list", mAvatarList,
        DB_READ_ARRAY_ELEMENT("index", int32, index)
        DB_READ_ARRAY_ELEMENT("active", int32, active)
    )
DB_DESERIALIZE_END
