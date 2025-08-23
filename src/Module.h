#pragma once

#include "Common.h"

#include <atomic>
#include <concepts>
#include <asio/io_context.hpp>


enum class EModuleState {
    CREATED,
    INITIALIZED,
    RUNNING,
    STOPPED,
};

/**
 * The Basic Class Of Server Module
 */
class BASE_API IModuleBase {

    friend class UServer;

    /** The Owner Server Pointer */
    UServer *mServer;

protected:
    /** Module Current State */
    std::atomic<EModuleState> mState;

protected:
    IModuleBase();

    void SetUpModule(UServer *server);

    virtual void Initial();
    virtual void Start();
    virtual void Stop();

public:
    virtual ~IModuleBase() = default;

    DISABLE_COPY_MOVE(IModuleBase)

    virtual const char *GetModuleName() const = 0;

    /// Call This Method After ::SetUpModule, It Will Return A Valid Pointer Or Throw Exception
    [[nodiscard]] UServer *GetServer() const;

    /// Get The IOContext Reference Of The Server's IOContext
    [[nodiscard]] asio::io_context& GetIOContext() const;

    /// Get The Module Current State
    [[nodiscard]] EModuleState GetState() const;
};

template<typename T>
concept CModuleType = std::derived_from<T, IModuleBase> && !std::is_same_v<T, IModuleBase>;

#define DECLARE_MODULE(module) \
private: \
    friend class UServer; \
    using Super = IModuleBase; \
    using ThisClass = module; \
public: \
    DISABLE_COPY_MOVE(module) \
private:

