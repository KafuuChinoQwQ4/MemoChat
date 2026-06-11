#include "AppCoordinators.h"

#include <QObject>
#include <utility>

RegisterCountdownController::RegisterCountdownController(RegisterCountdownPort port)
    : _port(std::move(port))
{
    QObject::connect(&_timer,
                     &QTimer::timeout,
                     [this]()
                     {
                         onRegisterCountdownTimeout();
                     });
}

void RegisterCountdownController::onRegisterCountdownTimeout()
{
    if (_port.countdownSeconds() > 0)
    {
        _port.setCountdownSeconds(_port.countdownSeconds() - 1);
    }

    if (_port.countdownSeconds() <= 0)
    {
        stopTimer();
        _port.setRegisterSuccessPage(false);
        _port.switchToLogin();
    }
}

void RegisterCountdownController::startTimer()
{
    _timer.start(1000);
}

void RegisterCountdownController::stopTimer()
{
    _timer.stop();
}
