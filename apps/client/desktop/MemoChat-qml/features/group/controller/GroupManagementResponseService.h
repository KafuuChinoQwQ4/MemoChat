#ifndef GROUPMANAGEMENTRESPONSESERVICE_H
#define GROUPMANAGEMENTRESPONSESERVICE_H

#include "global.h"

#include <QJsonObject>
#include <QString>
#include <QtGlobal>

#include <vector>

namespace memochat::group
{

struct GroupManagementResponseContext
{
    qint64 currentGroupId = 0;
    int dialogUidForCreatedGroup = 0;
};

struct GroupManagementResponseEffect
{
    QString statusText;
    bool statusIsError = false;
    bool hasStatus = false;

    bool refreshGroupList = false;
    bool refreshGroupModel = false;
    bool refreshDialogModel = false;

    bool selectDialogUid = false;
    int dialogUid = 0;

    bool emitGroupCreatedId = false;
    qint64 createdGroupId = 0;

    bool updateCurrentChatPeerIcon = false;
    QString currentChatPeerIcon;

    std::vector<qint64> removeGroupIds;
    std::vector<qint64> clearGroupConversationIds;

    bool handled = false;
};

class GroupManagementResponseService
{
public:
    static QString defaultGroupIcon();
    static QString normalizedGroupIcon(const QString& icon);

    static bool isManagementResponse(int reqId);
    static QString responseActionText(int reqId);
    static GroupManagementResponseEffect
    reduceSuccess(int reqId, const QJsonObject& payload, const GroupManagementResponseContext& context = {});
    static GroupManagementResponseEffect reduceError(int reqId, int error, const QJsonObject& payload = {});
};

} // namespace memochat::group

#endif // GROUPMANAGEMENTRESPONSESERVICE_H
