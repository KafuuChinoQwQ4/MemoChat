#include "AppController.h"
#include "AppCoordinators.h"

void AppController::requestRelationBootstrap()
{
    _session_coordinator->requestRelationBootstrap();
}

void AppController::onRelationBootstrapUpdated()
{
    _session_coordinator->onRelationBootstrapUpdated();
}
