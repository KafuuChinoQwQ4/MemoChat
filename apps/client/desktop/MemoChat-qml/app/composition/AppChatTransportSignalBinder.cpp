#include "AppController.h"

#include "AppChatConnectionCoordinator.h"
#include "IChatTransport.h"

void AppController::bindAppChatTransportSignals()
{
    const auto chatTransport = _gateway.chatTransport();

    connect(chatTransport.get(),
            &IChatTransport::sig_con_success,
            this,
            [this](bool success)
            {
                _chat_connection_coordinator->onTcpConnectFinished(success);
            });
    connect(chatTransport.get(),
            &IChatTransport::sig_connection_closed,
            this,
            [this]()
            {
                _chat_connection_coordinator->onConnectionClosed();
            });
}
