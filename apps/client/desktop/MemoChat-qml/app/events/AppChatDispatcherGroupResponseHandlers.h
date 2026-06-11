#pragma once

#include "global.h"

#include <QJsonObject>
#include <functional>

namespace memochat::app
{

struct AppChatDispatcherGroupResponseHandlers
{
    std::function<bool(ReqId, int, const QJsonObject&)> handleError;
    std::function<void(ReqId, const QJsonObject&)> handleManagement;
    std::function<void(const QJsonObject&)> handleHistory;
    std::function<void(ReqId, const QJsonObject&)> handleGroupMessageMutation;
    std::function<void(ReqId, const QJsonObject&)> handlePrivateMessageMutation;
    std::function<void(const QJsonObject&)> handlePrivateForward;
    std::function<void(ReqId, const QJsonObject&)> handleGroupMessageAck;
    std::function<void(ReqId, const QJsonObject&)> handleDialogMeta;
};

} // namespace memochat::app
