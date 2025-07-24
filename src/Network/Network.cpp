#include "Network.h"

#include <string>

static const std::string key = "1d0fa7879b018b71a00109e7e29eb0c5037617e9eddc3d46e95294fc1fa3ad50";

void UNetwork::Initial() {
    if (mState != EModuleState::CREATED)
        return;

    mState = EModuleState::INITIALIZED;
}

const unsigned char *UNetwork::GetEncryptionKey() {
    // TODO: Read Real Key From Config
    return HexToBytes(key).data();
}

std::shared_ptr<FPackage> UNetwork::BuildPackage() {
    return nullptr;
}

std::vector<uint8_t> UNetwork::HexToBytes(const std::string &hex) {
    std::vector<uint8_t> result;
    result.reserve(hex.size() / 2);
    for (size_t i = 0; i < hex.size(); i += 2) {
        result.push_back(static_cast<uint8_t>(std::stoi(hex.substr(i, 2), nullptr, 16)));
    }
    return result;
}
