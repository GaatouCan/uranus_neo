#include "GameWorld.h"
#include <Config/Config.h>


UGameWorld::UGameWorld() {
    bUpdatePerTick = true;

}

UGameWorld::~UGameWorld() {
}


awaitable<bool> UGameWorld::AsyncInitial(const IDataAsset_Interface *data) {
    if (const auto ret = co_await IServiceBase::AsyncInitial(data); !ret)
        co_return false;

    co_return true;
}

void UGameWorld::OnEvent(const std::shared_ptr<IEventParam_Interface> &event) {
    IServiceBase::OnEvent(event);
}

void UGameWorld::OnUpdate(ASteadyTimePoint now, ASteadyDuration delta) {
    IServiceBase::OnUpdate(now, delta);
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

extern "C" {
    SERVICE_API IServiceBase *CreateInstance() {
        return new UGameWorld();
    }

    SERVICE_API void DestroyInstance(IServiceBase *service) {
        delete dynamic_cast<UGameWorld *>(service);
    }
}
