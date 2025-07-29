#include <Server.h>
#include <Config/Config.h>
#include <Login/LoginAuth.h>
#include <Event/EventModule.h>
#include <Timer/TimerModule.h>
#include <Monitor/Monitor.h>
#include <Service/ServiceModule.h>
#include <Gateway/Gateway.h>
#include <Network/Network.h>
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
    server->CreateModule<UMonitor>();
    server->CreateModule<UServiceModule>();
    server->CreateModule<UGateway>();
    server->CreateModule<UNetwork>();

    server->Initial();
    server->Run();
    server->Shutdown();

    delete server;
    spdlog::drop_all();

    return 0;
}
