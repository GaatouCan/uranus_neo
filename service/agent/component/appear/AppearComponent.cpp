#include "AppearComponent.h"
#include "../../Protocol.h"

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/array.hpp>

UAppearComponent::UAppearComponent()
    : mCurrentIndex(0) {

}

UAppearComponent::~UAppearComponent() {
}

// DB_SERIALIZE_BEGIN(UAppearComponent)
ADocumentValue UAppearComponent::Serialize() const {
    bsoncxx::builder::basic::document doc;
    doc.append(bsoncxx::builder::basic::kvp("version", GetComponentName()));
    //DB_WRITE_FIELD("current_index", mCurrentIndex);
    doc.append(bsoncxx::builder::basic::kvp("current_index", mCurrentIndex));
    // DB_WRITE_ARRAY("avatar_list", mAvatarList,
    //     DB_WRITE_ARRAY_ELEMENT("index", index),
    //     DB_WRITE_ARRAY_ELEMENT("active", active))
    {
        bsoncxx::builder::basic::array array_builder;
        for (const auto &val: mAvatarList) {
            array_builder.append(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("index", val.index),
                                                                        bsoncxx::builder::basic::kvp(
                                                                            "active", val.active)));
        }
        doc.append(bsoncxx::builder::basic::kvp("avatar_list", array_builder.extract()));
    }
    return doc.extract();
}

DB_DESERIALIZE_BEGIN(UAppearComponent)
    DB_READ_FIELD("current_index", int32, mCurrentIndex)
    DB_READ_ARRAY("avatar_list", mAvatarList,
        DB_READ_ARRAY_ELEMENT("index", int32, index)
        DB_READ_ARRAY_ELEMENT("active", int32, active)
    )
DB_DESERIALIZE_END


void protocol::AppearanceRequest(uint32_t pid, const std::shared_ptr<FPacket> &pkg, UPlayer *plr) {

}
