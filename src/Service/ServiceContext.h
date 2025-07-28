#pragma once

#include "ContextBase.h"
#include "ServiceType.h"

class UServiceModule;

class BASE_API UServiceContext final : public IContextBase {

    int32_t mServiceID;

    /** The Dynamic Library Filename Of This Service **/
    std::string mFilename;

    /** If This Service Is Core Service **/
    EServiceType mType;

public:
    UServiceContext();
    ~UServiceContext() override;

    void SetServiceID(int32_t sid);
    [[nodiscard]] int32_t GetServiceID() const override;

    void SetFilename(const std::string& filename);
    [[nodiscard]] const std::string& GetFilename() const;

    void SetServiceType(EServiceType type);
    [[nodiscard]] EServiceType GetServiceType() const;

    [[nodiscard]] UServiceModule* GetServiceModule() const;
};

