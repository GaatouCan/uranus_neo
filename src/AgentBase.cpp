#include "AgentBase.h"
#include "Server.h"
#include "base/Package.h"

#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>


void UChannelPackageNode::SetPackage(const FPackageHandle &pkg) {
    mPackage = pkg;
}

void UChannelPackageNode::Execute(IActorBase *pActor) const {
    if (pActor != nullptr && mPackage != nullptr) {
        pActor->OnPackage(mPackage.Get());
    }
}

void UChannelEventNode::SetEventParam(const shared_ptr<IEventParam_Interface> &event) {
    mEvent = event;
}

void UChannelTaskNode::SetTask(const AActorTask &task) {
    mTask = task;
}

void UChannelTaskNode::Execute(IActorBase *pActor) const {
    if (pActor != nullptr && mTask != nullptr) {
        std::invoke(mTask, pActor);
    }
}

void UChannelEventNode::Execute(IActorBase *pActor) const {
    if (pActor != nullptr && mEvent != nullptr) {
        pActor->OnEvent(mEvent.get());
    }
}

IAgentBase::IAgentBase(asio::io_context &context, const size_t channelSize)
    : mContext(context),
      mModule(nullptr),
      mChannel(mContext, channelSize),
      mTimerManager(mContext) {
}

IAgentBase::~IAgentBase() {
}

asio::io_context &IAgentBase::GetIOContext() const {
    return mContext;
}

UServer *IAgentBase::GetServer() const {
    if (mModule == nullptr)
        throw std::runtime_error(fmt::format("{} - Module Is Null Pointer", __FUNCTION__));
    return mModule->GetServer();
}

bool IAgentBase::Initial(IModuleBase *pModule, IDataAsset_Interface *pData) {
    mModule = pModule;

    // Create The Package Pool
    mPackagePool = mModule->GetServer()->CreateUniquePackagePool(mContext);
    mPackagePool->Initial();

    return true;
}

void IAgentBase::CleanUp() {
    // Implement In SubClass
}

void IAgentBase::PushPackage(const FPackageHandle &pkg) {
    if (!mChannel.is_open())
        return;

    if (pkg == nullptr)
        return;

    // Wrap The Package In Node
    auto node = make_unique<UChannelPackageNode>();
    node->SetPackage(pkg);

    SPDLOG_TRACE("{} - Agent[{:p}]", __FUNCTION__, static_cast<void *>(this));

    // Push To The Channel
    if (const auto ret = mChannel.try_send_via_dispatch(std::error_code{}, std::move(node)); !ret) {
        auto temp = make_unique<UChannelPackageNode>();
        temp->SetPackage(pkg);

        co_spawn(mContext, [self = shared_from_this(), node = std::move(temp)]() mutable -> awaitable<void> {
            co_await self->mChannel.async_send(std::error_code{}, std::move(node));
        }, detached);
    }
}

void IAgentBase::PushEvent(const shared_ptr<IEventParam_Interface> &event) {
    if (!mChannel.is_open())
        return;

    if (event == nullptr)
        return;

    // Wrap The Event In Node
    auto node = make_unique<UChannelEventNode>();
    node->SetEventParam(event);

    SPDLOG_TRACE("{} - Agent[{:p}]", __FUNCTION__, static_cast<void *>(this));

    // Push To The Channel
    if (const auto ret = mChannel.try_send_via_dispatch(std::error_code{}, std::move(node)); !ret) {
        auto temp = make_unique<UChannelEventNode>();
        temp->SetEventParam(event);

        co_spawn(mContext, [self = shared_from_this(), node = std::move(temp)]() mutable -> awaitable<void> {
            co_await self->mChannel.async_send(std::error_code{}, std::move(node));
        }, detached);
    }
}

void IAgentBase::PushTask(const AActorTask &task) {
    if (!mChannel.is_open())
        return;

    if (task == nullptr)
        return;

    // Wrap The Function In Node
    auto node = make_unique<UChannelTaskNode>();
    node->SetTask(task);

    SPDLOG_TRACE("{} - Agent[{:p}]", __FUNCTION__, static_cast<void *>(this));

    // Push To The Channel
    if (const auto ret = mChannel.try_send_via_dispatch(std::error_code{}, std::move(node)); !ret) {
        auto temp = make_unique<UChannelTaskNode>();
        temp->SetTask(task);

        co_spawn(mContext, [self = shared_from_this(), node = std::move(temp)]() mutable -> awaitable<void> {
            co_await self->mChannel.async_send(std::error_code{}, std::move(node));
        }, detached);
    }
}

FPackageHandle IAgentBase::BuildPackage() const {
    // If Something Is Not Assigned, Throw The Exception
    if (mModule == nullptr || mPackagePool == nullptr)
        throw std::runtime_error(fmt::format("{} - AgentBase[{:p}] Not Initialized",
            __FUNCTION__, static_cast<const void *>(this)));

    return mPackagePool->Acquire<IPackage_Interface>();
}

FTimerHandle IAgentBase::CreateTimer(const ATimerTask &task, const int delay, const int rate) {
    if (!mChannel.is_open())
        return {};

    return mTimerManager.CreateTimer(task, delay, rate);
}

void IAgentBase::CancelTimer(const int64_t tid) const {
    mTimerManager.CancelTimer(tid);
}

void IAgentBase::CancelAllTimers() {
    mTimerManager.CancelAll();
}


awaitable<void> IAgentBase::ProcessChannel() {
    SPDLOG_TRACE("{} - Agent[{:p}] Begin Process Channel", __FUNCTION__, static_cast<const void *>(this));

    try {
        // Looping Condition
        while (mChannel.is_open()) {
            auto [ec, node] = co_await mChannel.async_receive();
            if (ec || !mChannel.is_open())
                break;

            if (node == nullptr)
                continue;

            // Execute The Task
            if (auto *pActor = GetActor()) {
                node->Execute(pActor);
            }
        }

        SPDLOG_TRACE("{} - Agent[{:p}] Complete Process Channel, Begin Clean Up",
            __FUNCTION__, static_cast<const void *>(this));

        // Clean Up The Resource After The Looping Completes
        CleanUp();
    } catch (const std::exception &e) {
        SPDLOG_ERROR("{} - {}", __FUNCTION__, e.what());
    }
}
