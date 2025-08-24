#include "ServiceModule.h"
#include "Server.h"
#include "ServiceAgent.h"

#include <spdlog/spdlog.h>


UServiceModule::UServiceModule() {
}

UServiceModule::~UServiceModule() {
    Stop();
}

asio::io_context &UServiceModule::GetWorkerIOContext() {
    return mWorkerPool.GetIOContext();
}

void UServiceModule::Initial() {
    if (mState != EModuleState::CREATED)
        throw std::logic_error(std::format("{} - Module[{}] Not In CREATED State", __FUNCTION__, GetModuleName()));

    const auto &cfg = GetServer()->GetServerConfig();

    // Load The Service Shared Libraries
    mServiceFactory->LoadService();

    // Create All The Core Services Defined In Configureation
    for (const auto &val : cfg["services"]["core"]) {
        const auto path = val["path"].as<std::string>();
        const auto library = mServiceFactory->FindService("core." + path);
        if (!library.IsValid()) {
            SPDLOG_CRITICAL("Failed To Find Service[{}]", path);
            continue;
        }

        // Allocate A Service ID
        const auto sid = mAllocator.Allocate();

        // Check If Service ID Repeated
        if (mServiceMap.contains(sid)) [[unlikely]]
            throw std::logic_error(fmt::format("Allocate Same Service ID[{}]", sid));

        // Create An Agent For Service
        const auto agent = make_shared<UServiceAgent>(mWorkerPool.GetIOContext());

        // Set Up The Agent
        agent->SetUpServiceID(sid);
        agent->SetUpLibrary(library);

        // Initial The Agent
        if (!agent->Initial(this, nullptr)) {
            SPDLOG_CRITICAL("Failed To Initial Service: {}", path);
            mAllocator.Recycle(sid);
            agent->Stop();
            continue;
        }

        // Check If The Service Name Repeated
        const std::string name = agent->GetServiceName();
        if (mServiceNameMap.contains(name)) {
            SPDLOG_CRITICAL("Service[{}] Already Exists", name);
            agent->Stop();
            continue;
        }

        SPDLOG_INFO("Service[{}] Initialized", name);

        // Insert The Service And Its Name To The Maps
        mServiceMap.insert_or_assign(sid, agent);
        mServiceNameMap.insert_or_assign(name, sid);
    }

    const auto updateMs = cfg["services"]["update"].as<int>();

    // Start The Update Loop
    mTickTimer = make_unique<ASteadyTimer>(mWorkerPool.GetIOContext());
    co_spawn(mWorkerPool.GetIOContext(), UpdateLoop(updateMs), detached);

    // Start The Worker Pool
    mWorkerPool.Start(4);

    mState = EModuleState::INITIALIZED;
}

void UServiceModule::Start() {
    if (mState != EModuleState::INITIALIZED)
        throw std::logic_error(std::format("{} - Module[{}] Not In INITIALIZED State", __FUNCTION__, GetModuleName()));

    // Boot All The Core Service
    for (const auto &context : mServiceMap | std::views::values) {
        SPDLOG_INFO("Boot Service[{}]", context->GetServiceName());
        context->BootService();
    }
    
    mState = EModuleState::RUNNING;
}

void UServiceModule::Stop() {
    if (mState == EModuleState::STOPPED)
        return;

    mState = EModuleState::STOPPED;

    // Stop The Worker Pool
    mWorkerPool.Stop();

    // Stop The Update Timer
    if (mTickTimer != nullptr) {
        mTickTimer->cancel();
    }

    // Shutdown All The Core Service
    for (const auto &context : mServiceMap | std::views::values) {
        SPDLOG_INFO("Stop Service[{}]", context->GetServiceName());
        context->Stop();
    }

    mServiceMap.clear();
    mServiceNameMap.clear();
}

awaitable<void> UServiceModule::UpdateLoop(int ms) {
    ms = (ms < 10) ? 10 : ms;

    ASteadyTimePoint tickPoint = std::chrono::steady_clock::now();
    const ASteadyDuration delta = std::chrono::milliseconds(ms);

    try {
        while (mState == EModuleState::RUNNING) {
            tickPoint += delta;
            mTickTimer->expires_at(tickPoint);

            if (const auto [ec] = co_await mTickTimer->async_wait(); ec) {
                break;
            }

            std::unique_lock tickLock(mTickMutex);
            std::shared_lock serviceLock(mServiceMutex);

            for (auto tickIter = mTickerSet.begin(); tickIter != mTickerSet.end();) {
                if (const auto serviceIter = mServiceMap.find(*tickIter); serviceIter != mServiceMap.end()) {
                    serviceIter->second->PushTicker(tickPoint, delta);
                    ++tickIter;
                    continue;
                }

                // Erase The Expired Service ID
                mTickerSet.erase(tickIter++);
            }
        }
    } catch (const std::exception &e) {
        SPDLOG_ERROR("{} - {}", __FUNCTION__, e.what());
    }
}

shared_ptr<UServiceAgent> UServiceModule::FindService(const int64_t sid) const {
    if (mState != EModuleState::RUNNING)
        return nullptr;

    std::shared_lock lock(mServiceMutex);
    const auto iter = mServiceMap.find(sid);
    return iter == mServiceMap.end() ? nullptr : iter->second;
}

shared_ptr<UServiceAgent> UServiceModule::FindService(const std::string &name) const {
    if (mState != EModuleState::RUNNING)
        return nullptr;

    int64_t sid;

    // Find The Service ID
    {
        std::shared_lock lock(mServiceNameMutex);
        const auto iter = mServiceNameMap.find(name);
        if (iter == mServiceNameMap.end())
            return nullptr;

        sid = iter->second;
    }

    if (sid < 0)
        return nullptr;

    return FindService(sid);
}

void UServiceModule::BootService(const std::string &path, IDataAsset_Interface *pData) {
    if (mState != EModuleState::RUNNING || mServiceFactory == nullptr)
        return;

    const auto library = mServiceFactory->FindService("extend." + path);
    if (!library.IsValid()) {
        SPDLOG_ERROR("{} - Fail To Find Service Library[{}]", __FUNCTION__, path);
        return;
    }

    const auto sid = mAllocator.AllocateTS();

    // Check If The Service ID Is Valid
    {
        std::shared_lock lock(mServiceMutex);
        if (mServiceMap.contains(sid)) {
            SPDLOG_CRITICAL("{} - Service ID Repeated, [{}]", __FUNCTION__, sid);
            return;
        }
    }

    // Create The Agent For Service
    const auto agent = make_shared<UServiceAgent>(mWorkerPool.GetIOContext());

    // Set Up The Agent
    agent->SetUpServiceID(sid);
    agent->SetUpLibrary(library);

    // Initial The Service
    if (!agent->Initial(this, pData)) {
        SPDLOG_ERROR("{} - Service[{}] Initialize Fail", __FUNCTION__, sid);
        agent->Stop();
        mAllocator.RecycleTS(sid);
        return;
    }

    // Check If The Service Name Is Defined
    const std::string name = agent->GetServiceName();
    if (name.empty() || name == "UNKNOWN") {
        SPDLOG_ERROR("{} - Service[{}] Name Undefined", __FUNCTION__, sid);
        agent->Stop();
        mAllocator.RecycleTS(sid);
        return;
    }

    // Check If The Service Name Is Unique
    bool bRepeat = false;
    {
        std::shared_lock lock(mServiceNameMutex);
        if (mServiceNameMap.contains(name)) {
            bRepeat = true;
        }
    }

    // Try To Boot The Service
    if (bRepeat || !agent->BootService()) {
        SPDLOG_ERROR("{} - Service[{}] Name Repeated Or Fail To Boot", __FUNCTION__, name);
        agent->Stop();
        mAllocator.RecycleTS(sid);
        return;
    }

    SPDLOG_INFO("{} - Boot Service[{}] Successfully", __FUNCTION__, name);

    // Insert The Service And The Name To The Maps
    std::scoped_lock lock(mServiceMutex, mServiceNameMutex);
    mServiceMap.insert_or_assign(sid, agent);
    mServiceNameMap.insert_or_assign(name, sid);
}

void UServiceModule::ShutdownService(const int64_t sid) {
    if (mState != EModuleState::RUNNING)
        return;

    shared_ptr<UServiceAgent> context;

    // Erase From Service Map
    {
        std::unique_lock lock(mServiceMutex);
        const auto iter = mServiceMap.find(sid);
        if (iter == mServiceMap.end())
            return;

        context = iter->second;
        mServiceMap.erase(iter);
    }

    if (context == nullptr)
        return;

    const std::string name = context->GetServiceName();

    // Erase From Name Mapping
    {
        std::unique_lock lock(mServiceNameMutex);
        mServiceNameMap.erase(name);
    }

    // Erase From The Update Map
    {
        std::unique_lock lock(mTickMutex);
        mTickerSet.erase(sid);
    }

    // Recycle The Service ID
    mAllocator.RecycleTS(sid);

    SPDLOG_INFO("{} - Stop The Service[{}, {}]", __FUNCTION__, sid, name);
    context->Stop();
}

void UServiceModule::ShutdownService(const std::string &name) {
    if (mState != EModuleState::RUNNING)
        return;

    int64_t sid;

    // Get The Service ID And Erase From The Name Mapping
    {
        std::unique_lock lock(mServiceNameMutex);
        const auto iter = mServiceNameMap.find(name);
        if (iter == mServiceNameMap.end())
            return;

        sid = iter->second;
        mServiceNameMap.erase(iter);
    }

    if (sid < 0)
        return;

    shared_ptr<UServiceAgent> context;

    // Erase From The Service Map
    {
        std::unique_lock lock(mServiceMutex);
        const auto iter = mServiceMap.find(sid);
        if (iter == mServiceMap.end())
            return;

        context = iter->second;
        mServiceMap.erase(iter);
    }

    // Erase From The Update Map
    {
        std::unique_lock lock(mTickMutex);
        mTickerSet.erase(sid);
    }

    // Recycle The Service ID
    mAllocator.RecycleTS(sid);

    if (context == nullptr)
        return;

    // If The Target Name Not Equal
    if (context->GetServiceName() != name) {
        SPDLOG_CRITICAL("{} - Service[{}] Name Not Equal[{} - {}]", __FUNCTION__, sid, name, context->GetServiceName());
    }

    SPDLOG_INFO("{} - Stop The Service[{}, {}]", __FUNCTION__, sid, context->GetServiceName());
    context->Stop();
}

void UServiceModule::InsertTicker(const int64_t sid) {
    if (mState != EModuleState::RUNNING)
        return;

    if (sid < 0)
        return;

    std::unique_lock lock(mTickMutex);
    mTickerSet.insert(sid);
}

void UServiceModule::RemoveTicker(const int64_t sid) {
    if (mState != EModuleState::RUNNING)
        return;

    if (sid < 0)
        return;

    std::unique_lock lock(mTickMutex);
    mTickerSet.erase(sid);
}

std::map<int64_t, std::string> UServiceModule::GetAllServiceMap() const {
    if (mState != EModuleState::RUNNING)
        return {};

    std::map<int64_t, std::string> result;

    std::shared_lock lock(mServiceNameMutex);
    for (const auto &[name, sid] : mServiceNameMap) {
        result.emplace(sid, name);
    }

    return result;
}
