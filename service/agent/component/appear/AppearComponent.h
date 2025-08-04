#pragma once

#include "Gateway/PlayerComponent.h"

#include <vector>

struct FAvatarInfo {
    int index;
    bool active;
};

class UAppearComponent final : public IPlayerComponent {

    DECLARE_COMPONENT("appear", 1)

public:
    UAppearComponent();
    ~UAppearComponent() override;

    COMPONENT_SERIALIZATION

private:
    int mCurrentIndex;
    std::vector<FAvatarInfo> mAvatarList;
};
