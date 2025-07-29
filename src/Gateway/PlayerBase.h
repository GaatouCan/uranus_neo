#pragma once

#include "PlayerAgent.h"
#include "Base/ProtocolRoute.h"


class BASE_API UPlayerBase : public IPlayerAgent {

    typedef void(*APlayerProtocol)(uint32_t, const std::shared_ptr<FPackage> &, UPlayerBase *);

public:
    UPlayerBase();
    ~UPlayerBase() override;

    void OnPackage(const std::shared_ptr<FPackage> &pkg) override;


private:
    /** Protocol Route While Receive Package **/
    TProtocolRoute<APlayerProtocol> mRoute;
};

