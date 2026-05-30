#include "AppController.h"

#include <QJsonObject>

void AppController::onGroupRsp(ReqId reqId, int error, QJsonObject payload)
{
    if (error != ErrorCodes::SUCCESS)
    {
        handleGroupRspError(reqId, error, payload);
        return;
    }

    switch (reqId)
    {
        case ID_CREATE_GROUP_RSP:
        case ID_INVITE_GROUP_MEMBER_RSP:
        case ID_APPLY_JOIN_GROUP_RSP:
        case ID_REVIEW_GROUP_APPLY_RSP:
            handleGroupManagementRsp(reqId, payload);
            break;
        case ID_GROUP_HISTORY_RSP:
            handleGroupHistoryRsp(payload);
            break;
        case ID_EDIT_GROUP_MSG_RSP:
        case ID_REVOKE_GROUP_MSG_RSP:
            handleGroupMessageMutationRsp(reqId, payload);
            break;
        case ID_EDIT_PRIVATE_MSG_RSP:
        case ID_REVOKE_PRIVATE_MSG_RSP:
            handlePrivateMessageMutationRsp(reqId, payload);
            break;
        case ID_FORWARD_PRIVATE_MSG_RSP:
            handlePrivateForwardRsp(payload);
            break;
        case ID_UPDATE_GROUP_ANNOUNCEMENT_RSP:
        case ID_UPDATE_GROUP_ICON_RSP:
        case ID_MUTE_GROUP_MEMBER_RSP:
        case ID_SET_GROUP_ADMIN_RSP:
        case ID_KICK_GROUP_MEMBER_RSP:
        case ID_QUIT_GROUP_RSP:
        case ID_DISSOLVE_GROUP_RSP:
            handleGroupManagementRsp(reqId, payload);
            break;
        case ID_GROUP_CHAT_MSG_RSP:
        case ID_FORWARD_GROUP_MSG_RSP:
            handleGroupMessageAckRsp(reqId, payload);
            break;
        case ID_SYNC_DRAFT_RSP:
            handleDialogMetaRsp(reqId, payload);
            break;
        case ID_PIN_DIALOG_RSP:
            handleDialogMetaRsp(reqId, payload);
            break;
        case ID_GET_GROUP_LIST_RSP:
        default:
            break;
    }
}
