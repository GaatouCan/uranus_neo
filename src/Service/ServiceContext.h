#pragma once

#include "ContextBase.h"

class UServiceModule;

class BASE_API UServiceContext final : public IContextBase {

    int32_t mServiceID;

    /** The Dynamic Library Filename Of This Service **/
    std::string mFilename;

    /** If This Service Is Core Service **/
    bool bCore;

public:
    UServiceContext();
    ~UServiceContext() override;

    void SetServiceID(int32_t sid);
    [[nodiscard]] int32_t GetServiceID() const override;

    void SetFilename(const std::string& filename);
    [[nodiscard]] const std::string& GetFilename() const;

    void SetCoreFlag(bool core);
    [[nodiscard]] bool GetCoreFlag() const;

    [[nodiscard]] UServiceModule* GetServiceModule() const;
};

