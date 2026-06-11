#include "AppController.h"

void AppController::bindAppControllerSignals()
{
    bindAppHttpSignals();
    bindAppChatTransportSignals();
    bindAppChatDispatcherSignals();
    bindAppCallSignals();
    bindAppFeatureFacadeSignals();
    bindAppShellSignals();
    bindAppChatProjectionSignals();
    bindAppTimerSignals();
}
