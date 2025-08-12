#include "ServiceModule.h"
#include "ServiceContext.h"
#include "config/Config.h"
#include "Server.h"

#include <spdlog/spdlog.h>
#include <filesystem>
#include <ranges>


UServiceModule::UServiceModule() {
}

void UServiceModule::Initial() {
    if (mState != EModuleState::CREATED)
        return;

    const auto *configModule = GetServer()->GetModule<UConfig>();
    if (configModule == nullptr) {
        SPDLOG_ERROR("Config Module Not Found, Service Module Initialization Failed!");
        GetServer()->Shutdown();
        exit(-6);
    }

    const auto &config = configModule->GetServerConfig();

#ifdef __linux__
    const std::string prefix = "lib";
#endif

    SPDLOG_INFO("Loading Core Service...");

    if (config["service"] && config["service"]["core"]) {
        for (const auto &val: config["service"]["core"]) {
            const auto filename = val["name"].as<std::string>();
#if defined(_WIN32) || defined(_WIN64)
            const std::filesystem::path path = std::filesystem::path(CORE_SERVICE_DIRECTORY) / (filename + ".dll");
#else
            std::string linux_filename = filename;
            if (filename.compare(0, prefix.size(), prefix) != 0) {
                linux_filename = prefix + filename;
            }
            const std::filesystem::path path = std::filesystem::path(CORE_SERVICE_DIRECTORY) / (linux_filename + ".so");
#endif
            mCoreLibraryMap[filename] = FSharedLibrary{ path };
        }
    } else {
        for (const auto &entry: std::filesystem::directory_iterator(CORE_SERVICE_DIRECTORY)) {
#if defined(_WIN32) || defined(_WIN64)
            if (entry.is_regular_file() && entry.path().extension() == ".dll") {
                const auto filename = entry.path().stem().string();
#else
            if (entry.is_regular_file() && entry.path().extension() == ".so") {
                auto filename = entry.path().stem().string();
                if (filename.compare(0, prefix.size(), prefix) == 0) {
                    filename.erase(0, prefix.size());
                }
#endif
                mCoreLibraryMap[filename] = FSharedLibrary{ entry.path() };
            }
        }
    }

    SPDLOG_INFO("Loading Extend Service...");

    if (config["service"] && config["service"]["extend"]) {
        for (const auto &val: config["service"]["extend"]) {
            const auto filename = val["name"].as<std::string>();
#if defined(_WIN32) || defined(_WIN64)
            const std::filesystem::path path = std::filesystem::path(EXTEND_SERVICE_DIRECTORY) / (filename + ".dll");
#else
            std::string linux_filename = filename;
            if (filename.compare(0, prefix.size(), prefix) != 0) {
                linux_filename = prefix + filename;
            }
            const std::filesystem::path path = std::filesystem::path(EXTEND_SERVICE_DIRECTORY) / (linux_filename + ".so");
#endif
            mExtendLibraryMap[filename] = FSharedLibrary{ path };
        }
    } else {
        for (const auto &entry: std::filesystem::directory_iterator(EXTEND_SERVICE_DIRECTORY)) {
#if defined(_WIN32) || defined(_WIN64)
            if (entry.is_regular_file() && entry.path().extension() == ".dll") {
                const auto filename = entry.path().stem().string();
#else
            if (entry.is_regular_file() && entry.path().extension() == ".so") {
                auto filename = entry.path().stem().string();
                if (filename.compare(0, prefix.size(), prefix) == 0) {
                    filename.erase(0, prefix.size());
                }

#endif
                mExtendLibraryMap[filename] = FSharedLibrary{ entry.path() };
            }
        }
    }


    // Begin Initial Core Service

    for (const auto &[filename, node]: mCoreLibraryMap) {
        const auto sid = mAllocator.Allocate();
        const auto context = std::make_shared<UServiceContext>();

        context->SetUpModule(this);
        context->SetUpLibrary(node);
        context->SetUpServiceID(sid);
        context->SetFilename(filename);
        context->SetServiceType(EServiceType::CORE);

        if (context->Initial(nullptr)) {
            const auto name = context->GetServiceName();

            mServiceMap[sid] = context;
            mFilenameMapping[filename].insert(sid);
            mNameToServiceID[name] = sid;

            SPDLOG_INFO("Initialized Service[{}]", name);
        } else {
            SPDLOG_ERROR("Failed To Initialize Service[{}]", filename);
            GetServer()->Shutdown();
            exit(-8);
        }
    }

    mState = EModuleState::INITIALIZED;
}

void UServiceModule::Start() {
    if (mState != EModuleState::INITIALIZED)
        return;

    for (const auto &context: mServiceMap | std::views::values) {
        if (context->BootService()) {
            SPDLOG_INFO("Starting Core Service[{}]...", context->GetServiceName());
        } else {
            SPDLOG_ERROR("Failed To Start Core Service[{}]", context->GetServiceName());
            GetServer()->Shutdown();
            exit(-9);
        }
    }

    mState = EModuleState::RUNNING;
}

void UServiceModule::Stop() {
    if (mState == EModuleState::STOPPED)
        return;

    mState = EModuleState::STOPPED;

    SPDLOG_INFO("Unloading All Service...");
    for (const auto &context: mServiceMap | std::views::values) {
        context->ForceShutdown();
    }

    mCoreLibraryMap.clear();
    mExtendLibraryMap.clear();

    SPDLOG_INFO("Free All Service Library Successfully");
}

UServiceModule::~UServiceModule() {
    Stop();
}

std::shared_ptr<UServiceContext> UServiceModule::FindService(const FServiceHandle sid) const {
    if (mState != EModuleState::RUNNING)
        return nullptr;

    std::shared_lock lock(mServiceMutex);
    const auto iter = mServiceMap.find(sid);
    return iter != mServiceMap.end() ? iter->second : nullptr;
}

std::shared_ptr<UServiceContext> UServiceModule::FindService(const std::string &name) const {
    if (mState != EModuleState::RUNNING)
        return nullptr;

    const FServiceHandle sid = GetServiceID(name);
    if (sid <= 0)
        return nullptr;

    return FindService(sid);
}

std::map<std::string, FServiceHandle> UServiceModule::GetServiceList() const {
    if (mState != EModuleState::RUNNING)
        return {};

    std::shared_lock lock(mServiceMutex);
    std::map<std::string, FServiceHandle> result;
    for (const auto &[sid, context] : mServiceMap) {
        result.emplace(context->GetServiceName(), sid);
    }
    return result;
}

FServiceHandle UServiceModule::GetServiceID(const std::string &name) const {
    if (mState != EModuleState::RUNNING)
        return INVALID_SERVICE_ID;

    std::shared_lock lock(mNameMutex);
    const auto nameIter = mNameToServiceID.find(name);
    if (nameIter == mNameToServiceID.end())
        return INVALID_SERVICE_ID;

    return nameIter->second;
}

FSharedLibrary UServiceModule::FindServiceLibrary(const std::string &filename, const EServiceType type) const {
    std::shared_lock lock(mLibraryMutex);
    if (type == EServiceType::CORE) {
        const auto iter = mCoreLibraryMap.find(filename);
        return iter != mCoreLibraryMap.end() ? iter->second : FSharedLibrary{};
    }

    const auto iter = mExtendLibraryMap.find(filename);
    return iter != mExtendLibraryMap.end() ? iter->second : FSharedLibrary{};
}

void UServiceModule::BootExtendService(const std::string &filename, const IDataAsset_Interface *data) {
    if (mState != EModuleState::RUNNING)
        return;

    const auto handle = FindServiceLibrary(filename);
    if (!handle.IsValid()) {
        SPDLOG_WARN("{:<20} - Cannot Found Service Library[{}]", __FUNCTION__, filename);
        return;
    }

    const auto sid = mAllocator.AllocateTS();
    if (sid < 0)
        return;

    auto context = std::make_shared<UServiceContext>();

    context->SetUpModule(this);
    context->SetUpLibrary(handle);
    context->SetUpServiceID(sid);
    context->SetFilename(filename);
    context->SetServiceType(EServiceType::EXTEND);

    co_spawn(GetServer()->GetIOContext(), [this, context, sid, data, filename, func = __FUNCTION__]() mutable -> awaitable<void> {
        if (const auto ret = co_await context->AsyncInitial(data); !ret) {
            SPDLOG_ERROR("{:<20} - Failed To Initial Service[{}]", func, filename);
            context->ForceShutdown();
            co_return;
        }

        // Check If Service Name Unique
        bool bSuccess = true;

        {
            std::shared_lock lock(mNameMutex);
            if (mNameToServiceID.contains(context->GetServiceName())) {
                SPDLOG_WARN("{:<20} - Service[{}] Has Already Exist.", func, context->GetServiceName());
                bSuccess = false;
            }
        }

        // If Service Name Unique
        if (bSuccess) {
            if (context->BootService()) {
                {
                    const auto name = context->GetServiceName();

                    std::scoped_lock lock(mServiceMutex, mFileNameMutex, mNameMutex);

                    mServiceMap[sid] = context;
                    mFilenameMapping[filename].insert(sid);
                    mNameToServiceID[name] = sid;
                }

                SPDLOG_INFO("{:<20} - Boot Extend Service[{}] Successfully", func, context->GetServiceName());
                co_return;
            }

            SPDLOG_ERROR("{:<20} - Failed To Boot Extend Service[{}]", func, context->GetServiceName());
        }

        // If Not Unique, Service Force To Shut Down And Recycle The Service ID
        context->ForceShutdown();
        // mAllocator.RecycleTS(sid);
    }, detached);
}

void UServiceModule::ShutdownService(const FServiceHandle sid) {
    if (mState != EModuleState::RUNNING)
        return;

    std::shared_ptr<UServiceContext> context = nullptr;

    // Find The Target Service Context
    {
        std::unique_lock lock(mServiceMutex);
        if (const auto node = mServiceMap.extract(sid); !node.empty()) {
            context = node.mapped();
        }
    }

    if (context == nullptr || context->GetServiceID() == INVALID_SERVICE_ID) {
        SPDLOG_ERROR("{:<20} - Can't Find Service[{}]", __FUNCTION__, static_cast<int>(sid));
        return;
    }

    // Erase Name To Service ID Mapping
    {
        std::unique_lock lock(mNameMutex);
        mNameToServiceID.erase(context->GetServiceName());
    }

    switch (context->GetServiceType()) {
        case EServiceType::CORE: {
        }
        break;
        case EServiceType::EXTEND: {
            auto func = [this, sid, filename = context->GetFilename()](UContextBase *) {
                OnServiceShutdown(filename, sid, EServiceType::EXTEND);
                // mAllocator.RecycleTS(sid);
            };

            // Try To Shut Down Service In 5 Seconds, If Timeout, Force Shut Down It
            // If Force Or Not, Run The Callback After Service Shutting Down
            context->Shutdown(false, 5, func);
        }
        break;
        default: break;
    }
}

FServiceHandle UServiceModule::AcquireServiceID() {
    return mAllocator.AllocateTS();
}

void UServiceModule::RecycleServiceID(const FServiceHandle id) {
    if (id <= 0)
        return;
    mAllocator.RecycleTS(id);
}

void UServiceModule::OnServiceShutdown(const std::string &filename, const FServiceHandle sid, const EServiceType type) {
    if (GetState() != EModuleState::RUNNING)
        return;

    if (sid <= 0)
        return;

    switch (type) {
        case EServiceType::CORE: {
            // TODO
        }
        break;
        case EServiceType::EXTEND: {
            std::unique_lock lock(mFileNameMutex);
            if (const auto iter = mFilenameMapping.find(filename); iter != mFilenameMapping.end()) {
                iter->second.erase(sid);
                if (iter->second.empty()) {
                    mFilenameMapping.erase(iter);
                }
            }
        }
        default: break;
    }
}
