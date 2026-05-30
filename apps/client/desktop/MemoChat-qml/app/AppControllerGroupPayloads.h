#ifndef APPCONTROLLERGROUPPAYLOADS_H
#define APPCONTROLLERGROUPPAYLOADS_H

#include "global.h"

#include <QByteArray>
#include <QString>
#include <QtGlobal>

namespace memochat::app_group_payloads
{

QByteArray buildRefreshGroupListPayload(int fromUid);
QByteArray buildRequestDialogListPayload(int fromUid);
QByteArray
buildInviteGroupMemberPayload(int fromUid, const QString& targetUserId, qint64 groupId, const QString& reason);
QByteArray buildApplyJoinGroupPayload(int fromUid, const QString& groupCode, const QString& reason);
QByteArray
buildEditMessagePayload(int fromUid, const QString& msgId, const QString& content, qint64 groupId, int peerUid);
QByteArray buildRevokeMessagePayload(int fromUid, const QString& msgId, qint64 groupId, int peerUid);
QByteArray buildForwardMessagePayload(int fromUid, const QString& msgId, qint64 groupId, int peerUid);
QByteArray buildGroupHistoryPayload(int fromUid, qint64 groupId, qint64 beforeSeq, int limit);
QString groupActionText(ReqId id);

} // namespace memochat::app_group_payloads

#endif // APPCONTROLLERGROUPPAYLOADS_H
