#include "AppController.h"
#include "AppCoordinators.h"

void AppController::onRegisterCountdownTimeout()
{
    _session_coordinator->onRegisterCountdownTimeout();
}
