#pragma once

#include "Common.h"

#include <atomic>
#include <concepts>


enum class BASE_API EModuleState {
    CREATED,
    INITIALIZED,
    RUNNING,
    STOPPED,
};

/**
 * The Base Class Of Server Module
 */
class BASE_API IModuleBase {

    friend class UServer;

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

    [[nodiscard]] UServer *GetServer() const;
    [[nodiscard]] EModuleState GetState() const;

private:
    /** The Owner Server Pointer */
    UServer *mServer;

protected:
    /** Module Current State */
    std::atomic<EModuleState> mState;
};

template<typename T>
concept CModuleType = std::derived_from<T, IModuleBase> && !std::is_same_v<T, IModuleBase>;

#define DECLARE_MODULE(module) \
private: \
    friend class UServer; \
    using Super = IModuleBase; \
public: \
    DISABLE_COPY_MOVE(module) \
private:

