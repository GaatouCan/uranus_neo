#pragma once

#include "Common.h"


class UServer;

class BASE_API IDataAsset_Interface {

public:
    virtual ~IDataAsset_Interface() = default;

    [[nodiscard]] virtual const char *GetTypeName() const = 0;
};
