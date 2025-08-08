#include "ServiceContext.h"

#include "Server.h"
#include "ServiceModule.h"


UServiceContext::UServiceContext()
    : mType(EServiceType::CORE) {
}

UServiceContext::~UServiceContext() {
}

// void UServiceContext::SetServiceID(const int32_t sid) {
//     mServiceID = sid;
// }

// int32_t UServiceContext::GetServiceID() const {
//     return mServiceID;
// }

void UServiceContext::SetFilename(const std::string &filename) {
    mFilename = filename;
}

const std::string &UServiceContext::GetFilename() const {
    return mFilename;
}

void UServiceContext::SetServiceType(const EServiceType type) {
    mType = type;
}

EServiceType UServiceContext::GetServiceType() const {
    return mType;
}


UServiceModule *UServiceContext::GetServiceModule() const {
    return dynamic_cast<UServiceModule *>(GetOwnerModule());
}

int32_t UServiceContext::GetOtherServiceID(const std::string &name) const {
    if (mState < EContextState::INITIALIZED || mState >= EContextState::WAITING)
        return -10;

    // Do Not Find Self
    if (name.empty() || name == GetServiceName())
        return -11;

    if (const auto *module = GetServiceModule()) {
        if (const auto sid = module->GetServiceID(name); sid != GetServiceID())
            return sid;
        return -14;
    }

    return -13;
}

std::shared_ptr<UServiceContext> UServiceContext::GetOtherService(const int32_t sid) const {
    if (mState < EContextState::INITIALIZED || mState >= EContextState::WAITING)
        return nullptr;

    if (sid < 0 || sid == mServiceID)
        return nullptr;

    if (const auto *module = GetServiceModule()) {
        return module->FindService(sid);
    }
    return nullptr;
}

std::shared_ptr<UServiceContext> UServiceContext::GetOtherService(const std::string &name) const {
    if (mState < EContextState::INITIALIZED || mState >= EContextState::WAITING)
        return nullptr;

    if (name.empty() || name == GetServiceName())
        return nullptr;

    if (const auto *module = GetServiceModule()) {
        return module->FindService(name);
    }

    return nullptr;
}
