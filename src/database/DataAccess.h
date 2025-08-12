#pragma once

#include "Module.h"
#include "base/ConcurrentDeque.h"
#include "DBAdapterBase.h"

#include <thread>
#include <vector>
#include <functional>
#include <yaml-cpp/yaml.h>


class IDBTaskBase;

class BASE_API UDataAccess final : public IModuleBase {

    DECLARE_MODULE(UDataAccess)

    friend class IDBAdapterBase;

protected:
    UDataAccess();

    void Initial() override;
    void Stop() override;

public:
    ~UDataAccess() override;

    [[nodiscard]] constexpr const char *GetModuleName() const override {
        return "Data Access";
    }

    template<class Type>
    requires std::derived_from<Type, IDBAdapterBase>
    void SetDatabaseAdapter();

    void SetStartUpConfig(const std::function<IDataAsset_Interface *(const YAML::Node &)> &func);

    [[nodiscard]] IDBAdapterBase *GetAdapter() const;

private:
    void PushTask(std::unique_ptr<IDBTaskBase> &&task);

private:
    std::unique_ptr<IDBAdapterBase> mAdapter;

    struct FWorkerNode {
        std::thread thread;
        TConcurrentDeque<std::unique_ptr<IDBTaskBase>, true> deque;
    };

    std::vector<FWorkerNode> mWorkerList;
    std::atomic_size_t mNextIndex;

    std::function<IDataAsset_Interface *(const YAML::Node &)> mInitConfig;
};


template<class Type> requires std::derived_from<Type, IDBAdapterBase>
inline void UDataAccess::SetDatabaseAdapter() {
    if (mState != EModuleState::CREATED)
        return;
    mAdapter = std::make_unique<Type>();
}

