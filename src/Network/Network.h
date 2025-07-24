#pragma once

#include "Module.h"

#include <memory>
#include <string>
#include <vector>

class FPackage;


class BASE_API UNetwork final : public IModuleBase {

    DECLARE_MODULE(UNetwork)

protected:
    void Initial() override;

public:
    [[nodiscard]] constexpr const char *GetModuleName() const override {
        return "Network";
    }

    const unsigned char* GetEncryptionKey();

    std::shared_ptr<FPackage> BuildPackage();

private:
    static std::vector<uint8_t> HexToBytes(const std::string &hex);
};

