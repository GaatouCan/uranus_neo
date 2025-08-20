#pragma once

#include "Common.h"

#include <memory>


class UServer;
class UGateway;
class UConnection;
using std::shared_ptr;
using std::weak_ptr;


class BASE_API UAgent final : public std::enable_shared_from_this<UAgent> {

    friend class UGateway;
    friend class UConnection;

public:
    UAgent();
    ~UAgent();

    DISABLE_COPY_MOVE(UAgent)

    [[nodiscard]] int64_t GetPlayerID() const;

    [[nodiscard]] UGateway *GetGateway() const;
    [[nodiscard]] UServer *GetUServer() const;

protected:
    void SetUpModule(UGateway *gateway);
    void SetUpConnection(const shared_ptr<UConnection> &conn);
    void SetUpPlayerID(int64_t pid);

    void OnRepeat();
    void OnLogout();

private:
    UGateway *mGateway;

    weak_ptr<UConnection> mConn;
    int64_t mPlayerID;
};
