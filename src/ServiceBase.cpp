#include "ServiceBase.h"

void IServiceBase::SetUpContext(IContextBase *context) {
}

IServiceBase::IServiceBase() {
}

IServiceBase::~IServiceBase() {
}

bool IServiceBase::Initial(const IDataAsset_Interface *data) {
}

bool IServiceBase::Start() {
}

void IServiceBase::Stop() {
}

void IServiceBase::OnPackage(const std::shared_ptr<FPackage> &pkg) {
}

void IServiceBase::OnEvent(const std::shared_ptr<IEventParam_Interface> &event) {
}
