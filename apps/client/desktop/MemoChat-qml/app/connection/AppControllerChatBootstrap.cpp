#include "AppController.h"
#include "AppCoordinators.h"

void AppController::beginPostLoginBootstrap()
{
    _session_coordinator->beginPostLoginBootstrap();
}

void AppController::runPostLoginBootstrap()
{
    _session_coordinator->runPostLoginBootstrap();
}
