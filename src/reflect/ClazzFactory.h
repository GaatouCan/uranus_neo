#pragma once

#include "Common.h"

#include <string>
#include <unordered_map>

class UClazz;

class BASE_API UClazzFactory final {

    UClazzFactory();

public:
    ~UClazzFactory();

    DISABLE_COPY_MOVE(UClazzFactory)

    static UClazzFactory &Instance();

    void RegisterClazz(UClazz *clazz);
    [[nodiscard]] UClazz *GetClazz(const std::string &name) const;

    static UClazz *FromName(const std::string &name);

private:
    std::unordered_map<std::string, UClazz *> mClazzMap;
};

