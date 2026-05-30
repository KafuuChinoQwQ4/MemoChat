#include "ChatMessageDispatcher.h"

#include <QDateTime>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

void ChatMessageDispatcher::registerSystemHandlers()
{
    _handlers.insert(ID_GET_DIALOG_LIST_RSP,
                     [this](ReqId id, int len, QByteArray data)
                     {
                         Q_UNUSED(len);
                         Q_UNUSED(id);
                         QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
                         if (jsonDoc.isNull() || !jsonDoc.isObject())
                         {
                             QJsonObject fallback;
                             fallback["error"] = ErrorCodes::ERR_JSON;
                             emit sig_dialog_list_rsp(fallback);
                             return;
                         }
                         emit sig_dialog_list_rsp(jsonDoc.object());
                     });

    _handlers.insert(ID_SYNC_DRAFT_RSP,
                     [this](ReqId id, int len, QByteArray data)
                     {
                         Q_UNUSED(len);
                         QJsonObject jsonObj;
                         parseGroupResponse(id, data, jsonObj);
                     });
    _handlers.insert(ID_PIN_DIALOG_RSP,
                     [this](ReqId id, int len, QByteArray data)
                     {
                         Q_UNUSED(len);
                         QJsonObject jsonObj;
                         parseGroupResponse(id, data, jsonObj);
                     });

    _handlers.insert(ID_NOTIFY_CALL_EVENT_REQ,
                     [this](ReqId id, int len, QByteArray data)
                     {
                         Q_UNUSED(id);
                         Q_UNUSED(len);
                         QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
                         if (jsonDoc.isNull() || !jsonDoc.isObject())
                         {
                             qWarning() << "call event parse failed";
                             return;
                         }
                         const QJsonObject jsonObj = jsonDoc.object();
                         if (jsonObj.value("error").toInt(ErrorCodes::SUCCESS) != ErrorCodes::SUCCESS)
                         {
                             return;
                         }
                         emit sig_call_event(jsonObj);
                     });

    _handlers.insert(ID_NOTIFY_OFF_LINE_REQ,
                     [this](ReqId id, int len, QByteArray data)
                     {
                         Q_UNUSED(id);
                         Q_UNUSED(len);
                         QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
                         if (jsonDoc.isNull())
                         {
                             return;
                         }
                         const QJsonObject jsonObj = jsonDoc.object();
                         if (jsonObj.value("error").toInt(ErrorCodes::ERR_JSON) != ErrorCodes::SUCCESS)
                         {
                             return;
                         }
                         emit sig_notify_offline();
                     });

    _handlers.insert(ID_HEARTBEAT_RSP,
                     [this](ReqId id, int len, QByteArray data)
                     {
                         Q_UNUSED(id);
                         Q_UNUSED(len);
                         QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
                         if (jsonDoc.isNull())
                         {
                             return;
                         }
                         const QJsonObject jsonObj = jsonDoc.object();
                         if (jsonObj.value("error").toInt(ErrorCodes::ERR_JSON) != ErrorCodes::SUCCESS)
                         {
                             return;
                         }
                         emit sig_heartbeat_ack(QDateTime::currentMSecsSinceEpoch());
                     });
}
