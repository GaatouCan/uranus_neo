#pragma once

#include "Module.h"
#include "ConcurrentDeque.h"

#include <thread>
#include <mongocxx/instance.hpp>
#include <mongocxx/client.hpp>


class BASE_API UDataAccess final : public IModuleBase {

    DECLARE_MODULE(UDataAccess)

protected:
    UDataAccess();

    void Initial() override;
    void Stop() override;

public:
    ~UDataAccess() override;

    [[nodiscard]] constexpr const char *GetModuleName() const override {
        return "Data Access";
    }

private:
    mongocxx::instance mInstance;
    mongocxx::client mClient;

    std::thread mThread;
    TConcurrentDeque<int, true> mDeque;
};

