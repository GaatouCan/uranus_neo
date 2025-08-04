#pragma once

#include "Gateway/PlayerComponent.h"

#include <vector>

struct FAvatarInfo {
    int index;
    bool active;
};

class UAppearComponent final : public IPlayerComponent {
public:
    UAppearComponent();
    ~UAppearComponent() override;

    [[nodiscard]] constexpr const char* GetComponentName() const override {
        return "Appear_V0";
    }

    DECLARE_COMPONENT_SERIALIZATION

private:
    int mCurrentIndex;
    std::vector<FAvatarInfo> mAvatarList;
};
