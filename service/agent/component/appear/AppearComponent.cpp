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

bsoncxx::document::value UAppearComponent::Serialize() const {
    bsoncxx::builder::basic::document doc;

    doc.append(bsoncxx::builder::basic::kvp("current_index", mCurrentIndex));

    {
        bsoncxx::builder::basic::array array_builder;
        for (const auto &val: mAvatarList) {
            array_builder.append(
                bsoncxx::builder::basic::make_document(
                    bsoncxx::builder::basic::kvp("index", val.index),
                    bsoncxx::builder::basic::kvp("active", val.active)
                )
            );
        }
        doc.append(bsoncxx::builder::basic::kvp("avatar_list", array_builder.extract()));
    }

    return doc.extract();
}

void UAppearComponent::Deserialize(const bsoncxx::document::value &doc) {
    if (const auto elem = doc["current_index"]; elem && elem.type() == bsoncxx::type::k_int32) {
        mCurrentIndex = elem.get_int32().value;
    }

    if (const auto array_elem = doc["avatar_list"]; array_elem && array_elem.type() == bsoncxx::type::k_array) {
        for (auto &&val: array_elem.get_array().value) {
            auto avatar_view = val.get_document().value;

            FAvatarInfo avatar;

            if (auto elem = avatar_view["index"]; elem && elem.type() == bsoncxx::type::k_int32) {
                avatar.index = elem.get_int32().value;
            }
            if (auto elem = avatar_view["active"]; elem && elem.type() == bsoncxx::type::k_bool) {
                avatar.active = elem.get_bool().value;
            }

            mAvatarList.push_back(avatar);
        }
    }
}
