#include "AppController.h"

void AppController::bindAppControllerSignals()
{
    bindAppHttpSignals();
    bindAppChatTransportSignals();
    bindAppChatDispatcherSignals();
    bindAppCallSignals();
    bindAppShellSignals();
    bindAppTimerSignals();
}
