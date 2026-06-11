#include "AppController.h"
#include "AppPortRegistry.h"

void AppController::bindAppControllerPorts()
{
    _port_registry->bindSessionPorts();
    _port_registry->bindMediaPorts();
    _port_registry->bindCallPorts();
    _port_registry->bindFeaturePorts();
    _port_registry->bindConnectionPorts();
}
