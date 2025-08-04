#pragma once

#include "Gateway/PlayerComponent.h"

#include <vector>
#include <bsoncxx/document/value.hpp>

struct FAvatarInfo {
    int index;
    bool active;
};

class UAppearComponent final : public IPlayerComponent {


public:
    UAppearComponent();
    ~UAppearComponent() override;

    bsoncxx::document::value Serialize() const;
    void Deserialize(const bsoncxx::document::value& doc);

private:
    int mCurrentIndex;
    std::vector<FAvatarInfo> mAvatarList;
};

