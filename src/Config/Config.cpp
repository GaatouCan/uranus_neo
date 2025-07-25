#include "Config.h"

UConfig::UConfig() {
}

void UConfig::Initial() {
}

UConfig::~UConfig() {
}

const YAML::Node &UConfig::GetServerConfig() const {
    return mServerConfig;
}
