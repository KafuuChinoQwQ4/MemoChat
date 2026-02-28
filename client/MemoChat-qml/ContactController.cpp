#include "ContactController.h"
#include "ClientGateway.h"
#include "tcpmgr.h"
#include "global.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace {
QJsonArray buildLabelsArray(const QVariantList &labels)
{
    QJsonArray labelsArray;
    for (const auto &labelVar : labels) {
        const QString label = labelVar.toString().trimmed();
        if (!label.isEmpty()) {
            labelsArray.append(label);
        }
    }
    return labelsArray;
}
}

ContactController::ContactController(ClientGateway *gateway)
    : _gateway(gateway)
{
}

bool ContactController::sendSearchUser(const QString &uidText, QString *errorText) const
{
    bool ok = false;
    const int uid = uidText.trimmed().toInt(&ok);
    if (!ok || uid <= 0) {
        if (errorText) {
            *errorText = "请输入有效 UID";
        }
        return false;
    }

    QJsonObject jsonObj;
    jsonObj["uid"] = uidText.trimmed();
    const QByteArray jsonData = QJsonDocument(jsonObj).toJson(QJsonDocument::Compact);
    _gateway->tcpMgr()->slot_send_data(ReqId::ID_SEARCH_USER_REQ, jsonData);
    return true;
}

void ContactController::sendAddFriend(int selfUid, const QString &selfName, int targetUid,
                                      const QString &bakName, const QVariantList &labels) const
{
    QJsonObject jsonObj;
    jsonObj["uid"] = selfUid;
    jsonObj["applyname"] = selfName;
    jsonObj["bakname"] = bakName.trimmed().isEmpty() ? selfName : bakName.trimmed();
    jsonObj["touid"] = targetUid;
    jsonObj["labels"] = buildLabelsArray(labels);

    const QByteArray jsonData = QJsonDocument(jsonObj).toJson(QJsonDocument::Compact);
    _gateway->tcpMgr()->slot_send_data(ReqId::ID_ADD_FRIEND_REQ, jsonData);
}

void ContactController::sendApproveFriend(int selfUid, int targetUid, const QString &remark,
                                          const QVariantList &labels) const
{
    QJsonObject jsonObj;
    jsonObj["fromuid"] = selfUid;
    jsonObj["touid"] = targetUid;
    jsonObj["back"] = remark;
    jsonObj["labels"] = buildLabelsArray(labels);

    const QByteArray jsonData = QJsonDocument(jsonObj).toJson(QJsonDocument::Compact);
    _gateway->tcpMgr()->slot_send_data(ReqId::ID_AUTH_FRIEND_REQ, jsonData);
}
