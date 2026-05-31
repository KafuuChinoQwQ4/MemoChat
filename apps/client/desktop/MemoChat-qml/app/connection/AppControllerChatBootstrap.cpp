#include "AppController.h"
#include "AppCoordinators.h"

void AppController::onSwitchToChat()
{
    _session_coordinator->onSwitchToChat();
}

void AppController::beginPostLoginBootstrap()
{
    _session_coordinator->beginPostLoginBootstrap();
}

void AppController::runPostLoginBootstrap()
{
    _session_coordinator->runPostLoginBootstrap();
}
