#include "ContactRequestPayloads.h"

#include <QJsonArray>
#include <QRegularExpression>

namespace
{
QJsonArray buildLabelsArray(const QVariantList& labels)
{
    QJsonArray labelsArray;
    for (const auto& labelVar : labels)
    {
        const QString label = labelVar.toString().trimmed();
        if (!label.isEmpty())
        {
            labelsArray.append(label);
        }
    }
    return labelsArray;
}
} // namespace

namespace memochat::contact_payload
{
bool isValidUserId(const QString& uidText)
{
    static const QRegularExpression kUserIdPattern("^u[1-9][0-9]{8}$");
    return kUserIdPattern.match(uidText.trimmed()).hasMatch();
}

QJsonObject buildSearchUserPayload(const QString& uidText)
{
    QJsonObject payload;
    payload["user_id"] = uidText.trimmed();
    return payload;
}

QJsonObject buildAddFriendPayload(int selfUid,
                                  const QString& selfName,
                                  int targetUid,
                                  const QString& bakName,
                                  const QVariantList& labels)
{
    QJsonObject payload;
    payload["uid"] = selfUid;
    payload["applyname"] = selfName;
    payload["bakname"] = bakName.trimmed().isEmpty() ? selfName : bakName.trimmed();
    payload["touid"] = targetUid;
    payload["labels"] = buildLabelsArray(labels);
    return payload;
}

QJsonObject buildApproveFriendPayload(int selfUid, int targetUid, const QString& remark, const QVariantList& labels)
{
    QJsonObject payload;
    payload["fromuid"] = selfUid;
    payload["touid"] = targetUid;
    payload["back"] = remark;
    payload["labels"] = buildLabelsArray(labels);
    return payload;
}

QJsonObject buildDeleteFriendPayload(int selfUid, int friendUid)
{
    QJsonObject payload;
    payload["fromuid"] = selfUid;
    payload["friend_uid"] = friendUid;
    return payload;
}
} // namespace memochat::contact_payload
