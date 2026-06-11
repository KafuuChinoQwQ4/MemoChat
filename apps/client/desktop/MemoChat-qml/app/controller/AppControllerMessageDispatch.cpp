#include "AppController.h"

#include "IChatTransport.h"

bool AppController::isChatTransportReady() const
{
    const auto transport = _gateway.chatTransport();
    return transport && transport->isConnected();
}
