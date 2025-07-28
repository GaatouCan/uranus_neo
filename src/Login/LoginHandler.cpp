#include "LoginHandler.h"
#include "LoginAuth.h"

ILoginHandler::ILoginHandler(ULoginAuth *module)
    : mLoginAuth(module) {
}

ILoginHandler::~ILoginHandler() {
}

UServer *ILoginHandler::GetServer() const {
    return mLoginAuth->GetServer();
}
