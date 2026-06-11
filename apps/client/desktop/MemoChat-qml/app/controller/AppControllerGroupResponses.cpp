#include "AppController.h"

#include "AppChatDispatcherGroupResponseHandlers.h"

#include <QJsonObject>

memochat::app::AppChatDispatcherGroupResponseHandlers AppController::groupResponseHandlers()
{
    memochat::app::AppChatDispatcherGroupResponseHandlers handlers;
    handlers.handleError = [this](ReqId reqId, int error, const QJsonObject& payload)
    {
        return handleGroupRspError(reqId, error, payload);
    };
    handlers.handleManagement = [this](ReqId reqId, const QJsonObject& payload)
    {
        handleGroupManagementRsp(reqId, payload);
    };
    handlers.handleHistory = [this](const QJsonObject& payload)
    {
        handleGroupHistoryRsp(payload);
    };
    handlers.handleGroupMessageMutation = [this](ReqId reqId, const QJsonObject& payload)
    {
        handleGroupMessageMutationRsp(reqId, payload);
    };
    handlers.handlePrivateMessageMutation = [this](ReqId reqId, const QJsonObject& payload)
    {
        handlePrivateMessageMutationRsp(reqId, payload);
    };
    handlers.handlePrivateForward = [this](const QJsonObject& payload)
    {
        handlePrivateForwardRsp(payload);
    };
    handlers.handleGroupMessageAck = [this](ReqId reqId, const QJsonObject& payload)
    {
        handleGroupMessageAckRsp(reqId, payload);
    };
    handlers.handleDialogMeta = [this](ReqId reqId, const QJsonObject& payload)
    {
        handleDialogMetaRsp(reqId, payload);
    };
    return handlers;
}
