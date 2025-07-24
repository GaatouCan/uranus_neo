#pragma once

#include "Module.h"


#include <memory>

class FPackage;


class BASE_API UNetwork final : public IModuleBase {

    DECLARE_MODULE(UNetwork)

protected:
    void Initial() override;

public:
    [[nodiscard]] constexpr const char *GetModuleName() const override {
        return "Network";
    }


    std::shared_ptr<FPackage> BuildPackage();
};

