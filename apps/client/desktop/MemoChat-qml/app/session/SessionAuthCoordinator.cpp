#include "AppCoordinators.h"

#include <QObject>
#include <utility>

SessionAuthCoordinator::SessionAuthCoordinator(SessionAuthPort port)
    : _port(std::move(port))
{
    QObject::connect(&_registerCodeCooldownTimer,
                     &QTimer::timeout,
                     [this]()
                     {
                         onRegisterCodeCooldownTimeout();
                     });
}

void SessionAuthCoordinator::startRegisterCodeCooldownTimer()
{
    _registerCodeCooldownTimer.start(1000);
}

void SessionAuthCoordinator::stopRegisterCodeCooldownTimer()
{
    _registerCodeCooldownTimer.stop();
}
