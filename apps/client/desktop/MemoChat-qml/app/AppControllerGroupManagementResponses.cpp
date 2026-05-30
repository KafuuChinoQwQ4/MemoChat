#include "AppController.h"
#include "ConversationSyncService.h"
#include "usermgr.h"

#include <QJsonObject>

void AppController::handleGroupManagementRsp(ReqId reqId, const QJsonObject& payload)
{
    switch (reqId)
    {
        case ID_CREATE_GROUP_RSP:
        {
            const QString groupName = payload.value("name").toString();
            const qint64 groupId = payload.value("groupid").toVariant().toLongLong();
            setGroupStatus(groupName.isEmpty() ? "群聊创建成功" : QString("群聊创建成功：%1").arg(groupName), false);
            if (groupId > 0)
            {
                refreshGroupModel();
                refreshDialogModel();
                const int dialogUid =
                    ConversationSyncService::resolveGroupDialogUid(_group_state.dialogUidMap, groupId);
                if (dialogUid != 0)
                {
                    selectDialogByUid(dialogUid);
                }
                emit groupCreated(groupId);
            }
            refreshGroupList();
            break;
        }
        case ID_INVITE_GROUP_MEMBER_RSP:
            setGroupStatus("邀请成员成功", false);
            refreshGroupList();
            break;
        case ID_APPLY_JOIN_GROUP_RSP:
            setGroupStatus("入群申请已提交", false);
            break;
        case ID_REVIEW_GROUP_APPLY_RSP:
            setGroupStatus("审核操作已完成", false);
            refreshGroupList();
            break;
        case ID_UPDATE_GROUP_ANNOUNCEMENT_RSP:
            setGroupStatus("群公告已更新", false);
            refreshGroupList();
            break;
        case ID_UPDATE_GROUP_ICON_RSP:
            if (_group_state.currentId == payload.value("groupid").toVariant().toLongLong())
            {
                const QString updatedGroupIcon =
                    payload.value("icon").toString().trimmed().isEmpty() ? QStringLiteral("qrc:/res/chat_icon.png")
                                                                         : payload.value("icon").toString();
                setCurrentChatPeerIcon(updatedGroupIcon);
            }
            setGroupStatus("群头像已更新", false);
            refreshGroupList();
            break;
        case ID_MUTE_GROUP_MEMBER_RSP:
            setGroupStatus("禁言设置已生效", false);
            break;
        case ID_SET_GROUP_ADMIN_RSP:
            setGroupStatus("管理员设置已更新", false);
            refreshGroupList();
            break;
        case ID_KICK_GROUP_MEMBER_RSP:
            setGroupStatus("成员已移出群聊", false);
            refreshGroupList();
            break;
        case ID_QUIT_GROUP_RSP:
            setGroupStatus("已退出当前群聊", false);
            _gateway.userMgr()->RemoveGroup(payload.value("groupid").toVariant().toLongLong());
            refreshGroupList();
            clearCurrentGroupConversation(payload.value("groupid").toVariant().toLongLong());
            break;
        case ID_DISSOLVE_GROUP_RSP:
        {
            const qint64 groupId = payload.value("groupid").toVariant().toLongLong();
            setGroupStatus("群聊已解散", false);
            _gateway.userMgr()->RemoveGroup(groupId);
            refreshGroupList();
            clearCurrentGroupConversation(groupId);
            break;
        }
        default:
            break;
    }
}
