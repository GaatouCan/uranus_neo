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

void UChannelEventNode::SetEventParam(const std::shared_ptr<IEventParam_Interface> &event) {
    mEvent = event;
}

void UChannelTaskNode::SetTask(const std::function<void(IActorBase *)> &task) {
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

IAgentBase::IAgentBase(asio::io_context &ctx)
    : mContext(ctx),
      mServer(nullptr),
      mTimerManager(mContext) {
}

IAgentBase::~IAgentBase() {
}

asio::io_context &IAgentBase::GetIOContext() const {
    return mContext;
}

UServer *IAgentBase::GetServer() const {
    return mServer;
}

bool IAgentBase::Initial(UServer *pServer, IDataAsset_Interface *pData) {
    mServer = pServer;

    // Create The Channel
    mChannel = make_unique<AChannel>(mContext, 1024);

    // Create The Package Pool
    mPackagePool = mServer->CreateUniquePackagePool(mContext);
    mPackagePool->Initial();

    return true;
}

void IAgentBase::CleanUp() {

}

void IAgentBase::PushPackage(const FPackageHandle &pkg) {
    if (mChannel == nullptr || !mChannel->is_open())
        return;

    if (pkg == nullptr)
        return;

    auto node = make_unique<UChannelPackageNode>();
    node->SetPackage(pkg);

    if (const auto ret = mChannel->try_send_via_dispatch(std::error_code{}, std::move(node)); !ret) {
        auto temp = make_unique<UChannelPackageNode>();
        temp->SetPackage(pkg);

        co_spawn(mContext, [self = shared_from_this(), node = std::move(temp)]() mutable -> awaitable<void> {
            co_await self->mChannel->async_send(std::error_code{}, std::move(node));
        }, detached);
    }
}

void IAgentBase::PushEvent(const shared_ptr<IEventParam_Interface> &event) {
    if (mChannel == nullptr || !mChannel->is_open())
        return;

    if (event == nullptr)
        return;

    auto node = make_unique<UChannelEventNode>();
    node->SetEventParam(event);

    if (const auto ret = mChannel->try_send_via_dispatch(std::error_code{}, std::move(node)); !ret) {
        auto temp = make_unique<UChannelEventNode>();
        temp->SetEventParam(event);

        co_spawn(mContext, [self = shared_from_this(), node = std::move(temp)]() mutable -> awaitable<void> {
            co_await self->mChannel->async_send(std::error_code{}, std::move(node));
        }, detached);
    }
}

void IAgentBase::PushTask(const std::function<void(IActorBase *)> &task) {
    if (mChannel == nullptr || !mChannel->is_open())
        return;

    if (task == nullptr)
        return;

    auto node = make_unique<UChannelTaskNode>();
    node->SetTask(task);

    if (const auto ret = mChannel->try_send_via_dispatch(std::error_code{}, std::move(node)); !ret) {
        auto temp = make_unique<UChannelTaskNode>();
        temp->SetTask(task);

        co_spawn(mContext, [self = shared_from_this(), node = std::move(temp)]() mutable -> awaitable<void> {
            co_await self->mChannel->async_send(std::error_code{}, std::move(node));
        }, detached);
    }
}

FPackageHandle IAgentBase::BuildPackage() const {
    if (mServer == nullptr || mChannel == nullptr || mPackagePool == nullptr)
        throw std::runtime_error(fmt::format("{} - AgentBase[{:p}] Not Initialized",
            __FUNCTION__, static_cast<const void *>(this)));

    return mPackagePool->Acquire<IPackage_Interface>();
}

FTimerHandle IAgentBase::CreateTimer(const ATimerTask &task, const int delay, const int rate) {
    if (mChannel == nullptr || !mChannel->is_open())
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
    if (mChannel == nullptr)
        co_return;

    try {
        while (mChannel->is_open()) {
            auto [ec, node] = co_await mChannel->async_receive();
            if (ec || !mChannel->is_open())
                break;

            if (node == nullptr)
                continue;

            const auto actor = GetActor();
            if (actor == nullptr)
                break;

            node->Execute(actor);
        }

        // Clean Up The Resource
        CleanUp();
    } catch (const std::exception &e) {
        SPDLOG_ERROR("{} - {}", __FUNCTION__, e.what());
    }
}
