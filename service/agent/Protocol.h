#pragma once

#include <memory>

class FPackage;
class UPlayerBase;

namespace protocol {
    void AppearanceRequest(uint32_t, const std::shared_ptr<FPackage> &, UPlayerBase *);
}
