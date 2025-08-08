#pragma once

#include "Module.h"
#include "Base/EventParam.h"
#include "Base/ContextHandle.h"

#include <unordered_map>
#include <unordered_set>
#include <shared_mutex>


class BASE_API UEventModule final : public IModuleBase {

    DECLARE_MODULE(UEventModule)

    using AListenerMap = std::unordered_map<int, std::unordered_set<FContextHandle, FContextHandle::FHash, FContextHandle::FEqual>>;

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

    void ListenEvent(const FContextHandle &handle, int event);

    void RemoveListenEvent(const FContextHandle &handle, int event);
    void RemoveListener(const FContextHandle &handle);

private:
    AListenerMap mListenerMap;
    mutable std::shared_mutex mListenerMutex;
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
