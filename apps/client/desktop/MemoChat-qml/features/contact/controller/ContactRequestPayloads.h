#ifndef CONTACTREQUESTPAYLOADS_H
#define CONTACTREQUESTPAYLOADS_H

#include <QJsonObject>
#include <QString>
#include <QVariantList>

namespace memochat::contact_payload
{
bool isValidUserId(const QString& uidText);
QJsonObject buildSearchUserPayload(const QString& uidText);
QJsonObject buildAddFriendPayload(int selfUid,
                                  const QString& selfName,
                                  int targetUid,
                                  const QString& bakName,
                                  const QVariantList& labels);
QJsonObject buildApproveFriendPayload(int selfUid, int targetUid, const QString& remark, const QVariantList& labels);
QJsonObject buildDeleteFriendPayload(int selfUid, int friendUid);
} // namespace memochat::contact_payload

#endif // CONTACTREQUESTPAYLOADS_H
