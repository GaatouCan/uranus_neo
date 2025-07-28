#include "ServiceContext.h"
#include "ServiceModule.h"

UServiceContext::UServiceContext()
    : mServiceID(INVALID_SERVICE_ID),
      mType(EServiceType::CORE) {
}

UServiceContext::~UServiceContext() {
}

void UServiceContext::SetServiceID(const int32_t sid) {
    mServiceID = sid;
}

int32_t UServiceContext::GetServiceID() const {
    return mServiceID;
}

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
