#include "AppCoordinators.h"

#include "AppController.h"

RegisterCountdownController::RegisterCountdownController(AppController& controller)
    : _app(controller)
{
}

void RegisterCountdownController::onRegisterCountdownTimeout()
{
    if (_app._shell_state.registerCountdown > 0)
    {
        --_app._shell_state.registerCountdown;
        emit _app.registerCountdownChanged();
    }

    if (_app._shell_state.registerCountdown <= 0)
    {
        _app._register_countdown_timer.stop();
        _app._shell_state.registerSuccessPage = false;
        emit _app.registerSuccessPageChanged();
        _app.switchToLogin();
    }
}
