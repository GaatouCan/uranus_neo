#include <Server.h>
#include <config/Config.h>
#include <login/LoginAuth.h>
#include <event/EventModule.h>
#include <timer/TickerModule.h>
#include <logger/LoggerModule.h>
#include <monitor/Monitor.h>
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
    server->CreateModule<UTickerModule>();
    server->CreateModule<ULoggerModule>();
    server->CreateModule<UMonitor>();

    // if (auto *dataAccess = server->CreateModule<UDataAccess>(); dataAccess != nullptr) {
    //     dataAccess->SetDatabaseAdapter<UMongoAdapter>();
    // }

    server->SetCodecFactory<UCodecFactory>();
    server->SetPlayerFactory<UPlayerFactory>();
    server->SetServiceFactory<UServiceFactory>();

    server->Initial();
    server->Start();
    server->Shutdown();

    delete server;
    spdlog::drop_all();

    return 0;
}
