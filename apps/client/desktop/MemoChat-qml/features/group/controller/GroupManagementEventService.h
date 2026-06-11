#ifndef GROUPMANAGEMENTEVENTSERVICE_H
#define GROUPMANAGEMENTEVENTSERVICE_H

#include <QJsonObject>
#include <QString>
#include <QtGlobal>
#include <vector>

namespace memochat::group
{

struct CurrentGroupSnapshot
{
    qint64 groupId = 0;
    QString name;
    QString code;
};

struct CurrentChatSnapshot
{
    qint64 currentGroupId = 0;
    int privatePeerUid = 0;
    bool hasCurrentGroupChat = false;
};

struct GroupInfoSnapshot
{
    qint64 groupId = 0;
    QString name;
    QString code;
    QString icon;
    bool exists = false;
};

struct GroupManagementEventContext
{
    int selfUid = 0;
    CurrentGroupSnapshot currentGroup;
    CurrentChatSnapshot currentChat;
    GroupInfoSnapshot refreshedCurrentGroup;
};

struct GroupManagementEffect
{
    QString statusText;
    bool statusIsError = false;
    bool hasStatus = false;

    bool requestGroupList = false;
    bool refreshGroupModel = false;
    bool refreshDialogModel = false;
    bool requestDialogList = false;
    bool refreshCurrentGroupModel = false;
    bool syncCurrentDialogDraft = false;
    bool loadCurrentChatMessages = false;
    bool clearMessageModel = false;
    bool resetCurrentChatProjection = false;

    bool selectCurrentGroup = false;
    qint64 selectedGroupId = 0;
    QString selectedGroupName;
    QString selectedGroupCode;

    bool clearCurrentGroup = false;
    bool openGroupConversation = false;
    qint64 openGroupId = 0;
    bool openGroupResetHistory = false;

    std::vector<qint64> removeGroupIds;
    std::vector<qint64> clearGroupConversationIds;

    bool clearCurrentChatIfGroupId = false;
    qint64 currentChatGroupIdToClear = 0;

    bool updateCurrentChatPeerIcon = false;
    QString currentChatPeerIcon;

    bool handled = false;
    bool chatConversationEvent = false;
};

class GroupManagementEventService
{
public:
    static QString defaultGroupIcon();
    static QString defaultPrivatePeerIcon();
    static QString normalizedGroupIcon(const QString& icon);

    static GroupManagementEffect reduceGroupListUpdated(const GroupManagementEventContext& context);
    static GroupManagementEffect reduceGroupInvite(qint64 groupId,
                                                   const QString& groupCode,
                                                   const QString& groupName,
                                                   int operatorUid,
                                                   const GroupManagementEventContext& context);
    static GroupManagementEffect
    reduceGroupApply(qint64 groupId, int applicantUid, const QString& applicantUserId, const QString& reason);
    static GroupManagementEffect reduceGroupMemberChanged(const QJsonObject& payload,
                                                          const GroupManagementEventContext& context);
};

} // namespace memochat::group

#endif // GROUPMANAGEMENTEVENTSERVICE_H
