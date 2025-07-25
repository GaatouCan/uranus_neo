#include "ServiceContext.h"
#include "ServiceModule.h"

UServiceContext::UServiceContext()
    : mServiceID(INVALID_SERVICE_ID),
      bCore(false) {
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

void UServiceContext::SetCoreFlag(const bool core) {
    bCore = core;
}

bool UServiceContext::GetCoreFlag() const {
    return bCore;
}

UServiceModule *UServiceContext::GetServiceModule() const {
    return dynamic_cast<UServiceModule *>(GetOwnerModule());
}
