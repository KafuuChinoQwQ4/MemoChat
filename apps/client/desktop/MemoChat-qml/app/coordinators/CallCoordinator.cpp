#include "AppCoordinators.h"

#include "CallController.h"

CallCoordinator::CallCoordinator(CallShellPort port)
    : _port(std::move(port))
{
}

CallShellSnapshot CallCoordinator::snapshot() const
{
    return _port.snapshot ? _port.snapshot() : CallShellSnapshot{};
}

CallController* CallCoordinator::callController() const
{
    return _port.callController;
}
