#ifndef GROUPREQUESTPAYLOADS_H
#define GROUPREQUESTPAYLOADS_H

#include "global.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QVariantList>
#include <QtGlobal>

namespace memochat::group_payload
{
inline constexpr qint64 kPermissionChangeGroupInfo = 1LL << 0;
inline constexpr qint64 kPermissionDeleteMessages = 1LL << 1;
inline constexpr qint64 kPermissionInviteUsers = 1LL << 2;
inline constexpr qint64 kPermissionManageAdmins = 1LL << 3;
inline constexpr qint64 kPermissionPinMessages = 1LL << 4;
inline constexpr qint64 kPermissionBanUsers = 1LL << 5;
inline constexpr qint64 kPermissionManageTopics = 1LL << 6;
inline constexpr qint64 kDefaultAdminPermissionBits = kPermissionChangeGroupInfo | kPermissionDeleteMessages |
                                                      kPermissionInviteUsers | kPermissionPinMessages |
                                                      kPermissionBanUsers;
inline constexpr qint64 kOwnerPermissionBits =
    kDefaultAdminPermissionBits | kPermissionManageAdmins | kPermissionManageTopics;

QString trimmedGroupName(const QString& name);
QString trimmedUserId(const QString& userId);
QString trimmedGroupCode(const QString& groupCode);
QString trimmedMessageId(const QString& msgId);
bool isValidUserId(const QString& userId);
bool isValidGroupCode(const QString& groupCode);
bool isValidGroupName(const QString& name);
QJsonArray buildMemberUserIdArray(const QVariantList& memberUserIdList,
                                  const QString& selfUserId,
                                  QString* invalidUserId = nullptr);
qint64 normalizeAdminPermissionBits(bool isAdmin, qint64 permissionBits);

QJsonObject buildRefreshGroupListPayload(int fromUid);
QJsonObject buildRequestDialogListPayload(int fromUid);
QJsonObject
buildCreateGroupPayload(int fromUid, const QString& name, const QJsonArray& memberUserIds, int memberLimit = 200);
QJsonObject
buildInviteGroupMemberPayload(int fromUid, const QString& targetUserId, qint64 groupId, const QString& reason);
QJsonObject buildApplyJoinGroupPayload(int fromUid, const QString& groupCode, const QString& reason);
QJsonObject buildReviewGroupApplyPayload(int fromUid, qint64 applyId, bool agree);
QJsonObject buildGroupIdPayload(int fromUid, qint64 groupId);
QJsonObject buildUpdateGroupAnnouncementPayload(int fromUid, qint64 groupId, const QString& announcement);
QJsonObject buildUpdateGroupIconPayload(int fromUid, qint64 groupId, const QString& icon);
QJsonObject buildSetGroupAdminPayload(int fromUid,
                                      qint64 groupId,
                                      const QString& targetUserId,
                                      bool isAdmin,
                                      qint64 permissionBits);
QJsonObject buildMuteGroupMemberPayload(int fromUid, qint64 groupId, const QString& targetUserId, int muteSeconds);
QJsonObject buildKickGroupMemberPayload(int fromUid, qint64 groupId, const QString& targetUserId);
QJsonObject buildGroupEditMessagePayload(int fromUid, qint64 groupId, const QString& msgId, const QString& content);
QJsonObject buildGroupRevokeMessagePayload(int fromUid, qint64 groupId, const QString& msgId);
QJsonObject buildGroupForwardMessagePayload(int fromUid, qint64 groupId, const QString& msgId);
QJsonObject buildGroupHistoryPayload(int fromUid, qint64 groupId, qint64 beforeSeq, int limit);
QString actionText(ReqId reqId);
} // namespace memochat::group_payload

#endif // GROUPREQUESTPAYLOADS_H
