#include <Server.h>
#include <Config/Config.h>
#include <Login/LoginAuth.h>
#include <Event/EventModule.h>
#include <Timer/TimerModule.h>
#include <Service/ServiceModule.h>
#include <Gateway/Gateway.h>
#include <Network/Network.h>

#include <spdlog/spdlog.h>

int main() {

    const auto server = new UServer();

    if (auto *config = server->CreateModule<UConfig>(); config != nullptr) {
        config->SetYAMLPath("../../config");
        config->SetJSONPath("../../config");
    }

    server->CreateModule<ULoginAuth>();
    server->CreateModule<UEventModule>();
    server->CreateModule<UTimerModule>();
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
