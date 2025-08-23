#pragma once

#include "Module.h"
#include "base/Types.h"
#include "base/EventParam.h"

#include <absl/container/flat_hash_map.h>
#include <memory>
#include <shared_mutex>


class IAgentBase;
using std::weak_ptr;


class BASE_API UEventModule final : public IModuleBase {

    DECLARE_MODULE(UEventModule)

    using AListenerMap = absl::flat_hash_map<int, absl::flat_hash_map<int64_t, weak_ptr<IAgentBase>>>;

protected:
    void Initial() override;
    void Stop() override;

public:
    UEventModule();
    ~UEventModule() override = default;

    [[nodiscard]] constexpr const char *GetModuleName() const override {
        return "Event Module";
    }

    template<CEventType Type>
    std::shared_ptr<Type> CreateEvent() const;

    void Dispatch(const std::shared_ptr<IEventParam_Interface> &event);

    template<CEventType Type, class... Args>
    void DispatchT(Args && ... args);

    void ServiceListenEvent(int64_t sid, const weak_ptr<IAgentBase> &weakPtr, int event);

    void RemoveServiceListenerByEvent(int64_t sid, int event);
    void RemoveServiceListener(int64_t sid);

    void PlayerListenEvent(int64_t pid, const weak_ptr<IAgentBase> &weakPtr, int event);

    void RemovePlayerListenerByEvent(int64_t pid, int event);
    void RemovePlayerListener(int64_t pid);

private:
    void RemoveExpiredServices();
    void RemoveExpiredPlayers();

private:
    AListenerMap mServiceListenerMap;
    mutable std::shared_mutex mServiceListenerMutex;

    std::unique_ptr<ASteadyTimer> mServiceTimer;

    AListenerMap mPlayerListenerMap;
    mutable std::shared_mutex mPlayerListenerMutex;

    std::unique_ptr<ASteadyTimer> mPlayerTimer;

    std::atomic_bool bServiceWaiting;
    std::atomic_bool bPlayerWaiting;
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
