#include <Server.h>
#include <config/Config.h>
#include <login/LoginAuth.h>
#include <event/EventModule.h>
#include <timer/TimerModule.h>
#include <logger/LoggerModule.h>
#include <monitor/Monitor.h>
#include <database/DataAccess.h>
#include <service/ServiceModule.h>
#include <gateway/Gateway.h>
#include <gateway/Gateway.h>
#include <internal/CodecFactory.h>
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
    server->CreateModule<UTimerModule>();
    server->CreateModule<ULoggerModule>();
    server->CreateModule<UMonitor>();

    // if (auto *dataAccess = server->CreateModule<UDataAccess>(); dataAccess != nullptr) {
    //     dataAccess->SetDatabaseAdapter<UMongoAdapter>();
    // }

    server->CreateModule<UServiceModule>();
    server->CreateModule<UGateway>();

    if (auto *network = server->CreateModule<UGateway>(); network != nullptr) {
        network->SetCodecFactory<UCodecFactory>();
    }

    server->Initial();
    server->Run();
    server->Shutdown();

    delete server;
    spdlog::drop_all();

    return 0;
}
