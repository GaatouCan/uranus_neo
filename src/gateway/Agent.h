#pragma once

#include "Common.h"
#include "base/Types.h"

#include <memory>
#include <atomic>


class UServer;
class UGateway;
class UConnection;
class IPlayerBase;
class IRecyclerBase;

using std::make_unique;
using std::shared_ptr;
using std::weak_ptr;
using std::unique_ptr;


enum class EAgentState {
    CREATED,
    INITIALIZED,
    IDLE,
    RUNNING,
    WAITING,
    TERMINATED,
};


class BASE_API UAgent final : public std::enable_shared_from_this<UAgent> {

    friend class UGateway;
    friend class UConnection;

#pragma region Scheduler Node

    class BASE_API ISchedule_Interface {

    public:
        ISchedule_Interface() = default;
        virtual ~ISchedule_Interface() = default;

        DISABLE_COPY_MOVE(ISchedule_Interface)

        virtual void Execute(IPlayerBase *pPlayer) const = 0;
    };

#pragma endregion

    using AAgentChannel = TConcurrentChannel<void(std::error_code, unique_ptr<ISchedule_Interface>)>;

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

    void Start();

    void OnRepeat();

    /** Called By Connection **/
    void OnLogout();

private:
    awaitable<void> AsyncBoot();
    awaitable<void> ProcessChannel();

    void Shutdown();

private:
    UGateway *mGateway;

    weak_ptr<UConnection>       mConn;
    unique_ptr<AAgentChannel>   mChannel;

    int64_t         mPlayerID;
    IPlayerBase *   mPlayer;

    std::atomic<EAgentState> mState;
};
