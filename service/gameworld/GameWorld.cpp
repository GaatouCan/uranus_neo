#include "GameWorld.h"
#include <Config/Config.h>


UGameWorld::UGameWorld() {

}

UGameWorld::~UGameWorld() {
}

bool UGameWorld::Initial(const IDataAsset_Interface *data) {
    IServiceBase::Initial(data);
    return true;
}

bool UGameWorld::Start() {
    IServiceBase::Start();
    return true;
}

void UGameWorld::Stop() {
    IServiceBase::Stop();
}

void UGameWorld::OnPackage(const std::shared_ptr<IPackage_Interface> &pkg) {

}

extern "C" SERVICE_API IServiceBase *CreateInstance() {
    return new UGameWorld();
}

extern "C" SERVICE_API void DestroyInstance(IServiceBase *service) {
    delete dynamic_cast<UGameWorld *>(service);
}
