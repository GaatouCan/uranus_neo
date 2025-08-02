#pragma once

#include "Module.h"
#include "ConcurrentDeque.h"

#include <thread>
#include <vector>
#include <memory>
#include <mongocxx/client.hpp>


struct FDataAccessNode {
    std::unique_ptr<std::thread> thread;
    std::unique_ptr<mongocxx::client> client;
    std::unique_ptr<TConcurrentDeque<int, true>> queue;
};


class BASE_API UDataAccess final : public IModuleBase {

    DECLARE_MODULE(UDataAccess)

protected:
    UDataAccess();

    void Initial() override;

public:
    ~UDataAccess() override;

    [[nodiscard]] constexpr const char *GetModuleName() const override {
        return "Data Access";
    }
};

