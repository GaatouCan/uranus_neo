#pragma once

#include "ContextBase.h"
#include "ServiceType.h"


class UServiceModule;


class BASE_API UServiceContext final : public UContextBase {

    /** The Dynamic Library Filename Of This Service **/
    std::string mFilename;

    /** If This Service Is Core Service **/
    EServiceType mType;

public:
    UServiceContext();
    ~UServiceContext() override = default;

    void SetFilename(const std::string &filename);
    [[nodiscard]] const std::string &GetFilename() const;

    void SetServiceType(EServiceType type);
    [[nodiscard]] EServiceType GetServiceType() const;

    [[nodiscard]] UServiceModule *GetServiceModule() const;

    [[nodiscard]] FServiceHandle GetOtherServiceID(const std::string &name) const;

    [[nodiscard]] shared_ptr<UServiceContext> GetOtherService(FServiceHandle sid) const;
    [[nodiscard]] shared_ptr<UServiceContext> GetOtherService(const std::string &name) const;
};
