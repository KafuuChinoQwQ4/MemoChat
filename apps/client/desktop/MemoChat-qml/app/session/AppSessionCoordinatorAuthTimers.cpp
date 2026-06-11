#include "AppCoordinators.h"

void AppSessionCoordinator::onRegisterCountdownTimeout()
{
    _register_countdown->onRegisterCountdownTimeout();
}

void AppSessionCoordinator::onRegisterCodeCooldownTimeout()
{
    _auth->onRegisterCodeCooldownTimeout();
}

void AppSessionCoordinator::stopRegisterCountdownTimer()
{
    _register_countdown->stopTimer();
}
