#include "AppController.h"

#include "AppChatConnectionCoordinator.h"

#include <QTimer>

void AppController::bindAppTimerSignals()
{
    connect(&_heartbeat_timer,
            &QTimer::timeout,
            this,
            [this]()
            {
                _chat_connection_coordinator->onHeartbeatTimeout();
            });
    _chat_login_timeout_timer.setSingleShot(true);
    _chat_login_timeout_timer.setInterval(15000);
    connect(&_chat_login_timeout_timer,
            &QTimer::timeout,
            this,
            [this]()
            {
                _chat_connection_coordinator->onLoginTimeout();
            });
}
