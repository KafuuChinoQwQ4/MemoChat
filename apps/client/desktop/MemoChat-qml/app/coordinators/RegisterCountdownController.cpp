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
        _app.setRegisterCountdown(_app._shell_state.registerCountdown - 1);
    }

    if (_app._shell_state.registerCountdown <= 0)
    {
        _app._register_countdown_timer.stop();
        _app.setRegisterSuccessPage(false);
        _app.switchToLogin();
    }
}
