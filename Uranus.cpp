#include <Server.h>
#include <config/Config.h>
#include <login/LoginAuth.h>
#include <event/EventModule.h>
#include <logger/LoggerModule.h>
#include <monitor/Monitor.h>
#include <gateway/Gateway.h>
#include <service/ServiceModule.h>
#include <route/RouteModule.h>
#include <internal/CodecFactory.h>
#include <internal/PlayerFactory.h>
#include <internal/ServiceFactory.h>
#include <LoginHandlerImpl.h>

#include <spdlog/spdlog.h>

int main() {
#ifdef _DEBUG
    spdlog::set_level(spdlog::level::trace);
#else
    spdlog::set_level(spdlog::level::info);
#endif

    const auto server = new UServer();

    if (auto *config = server->CreateModule<UConfig>(); config != nullptr) {
        config->SetYAMLPath("../../config");
        config->SetJSONPath("../../config");
    }

    if (auto *auth = server->CreateModule<ULoginAuth>(); auth != nullptr) {
        auth->SetLoginHandler<ULoginHandlerImpl>();
    }

    server->CreateModule<UEventModule>();
    server->CreateModule<URouteModule>();
    server->CreateModule<ULoggerModule>();
    server->CreateModule<UMonitor>();

    // if (auto *dataAccess = server->CreateModule<UDataAccess>(); dataAccess != nullptr) {
    //     dataAccess->SetDatabaseAdapter<UMongoAdapter>();
    // }

    if (auto *module = server->CreateModule<UServiceModule>(); module != nullptr) {
        module->SetServiceFactory<UServiceFactory>();
    }

    if (auto *gateway = server->CreateModule<UGateway>(); gateway != nullptr) {
        gateway->SetPlayerFactory<UPlayerFactory>();
    }

    server->SetCodecFactory<UCodecFactory>();

    server->Initial();
    server->Start();
    server->Shutdown();

    delete server;
    spdlog::drop_all();

    return 0;
}
