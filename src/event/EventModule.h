#pragma once

#include "Module.h"
#include "base/EventParam.h"
#include "base/ContextHandle.h"
#include "base/AgentHandle.h"

#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>
#include <shared_mutex>


class BASE_API UEventModule final : public IModuleBase {

    DECLARE_MODULE(UEventModule)

    using AServiceListenerMap = absl::flat_hash_map<int, absl::flat_hash_set<FContextHandle, FContextHandle::FHash, FContextHandle::FEqual>>;
    using APlayerListenerMap = absl::flat_hash_map<int, absl::flat_hash_set<FAgentHandle, FAgentHandle::FHash, FAgentHandle::FEqual>>;

protected:
    UEventModule();

public:
    ~UEventModule() override = default;

    constexpr const char *GetModuleName() const override {
        return "Event Module";
    }

    template<CEventType Type>
    std::shared_ptr<Type> CreateEvent() const;

    void Dispatch(const std::shared_ptr<IEventParam_Interface> &event);

    template<CEventType Type, class... Args>
    void DispatchT(Args && ... args);

    void ServiceListenEvent(const FContextHandle &handle, int event);

    void RemoveServiceListenerByEvent(const FContextHandle &handle, int event);
    void RemoveServiceListener(const FContextHandle &handle);

    void PlayerListenEvent(const FAgentHandle &handle, int event);

    void RemovePlayerListenerByEvent(const FAgentHandle &handle, int event);
    void RemovePlayerListener(const FAgentHandle &handle);

private:
    AServiceListenerMap mServiceListenerMap;
    mutable std::shared_mutex mServiceListenerMutex;

    APlayerListenerMap mPlayerListenerMap;
    mutable std::shared_mutex mPlayerListenerMutex;
};

template<CEventType Type>
inline std::shared_ptr<Type> UEventModule::CreateEvent() const {
    if (mState != EModuleState::RUNNING)
        return nullptr;

    auto result = std::make_shared<Type>();
    return result;
}

template<CEventType Type, class ... Args>
inline void UEventModule::DispatchT(Args &&...args) {
    if (mState != EModuleState::RUNNING)
        return;

    auto res = std::make_shared<Type>(std::forward<Args>(args)...);
    this->Dispatch(res);
}
