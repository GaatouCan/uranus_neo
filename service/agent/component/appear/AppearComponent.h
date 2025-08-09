#pragma once

#include "../../PlayerComponent.h"

#include <vector>

struct FAvatarInfo {
    int index;
    bool active;
};

class UAppearComponent final : public IPlayerComponent {

    DECLARE_COMPONENT(UAppearComponent)

public:
    UAppearComponent();
    ~UAppearComponent() override;

    [[nodiscard]] constexpr const char *GetComponentName() const override {
        return "Appear";
    }

    [[nodiscard]] constexpr int GetComponentVersion() const override {
        return 1;
    }

private:
    int mCurrentIndex;
    std::vector<FAvatarInfo> mAvatarList;
};
