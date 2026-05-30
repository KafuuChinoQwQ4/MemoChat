#include "ChatMessageDispatcher.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

void ChatMessageDispatcher::registerPrivateMessageHandlers()
{
    _handlers.insert(ID_TEXT_CHAT_MSG_RSP,
                     [this](ReqId id, int len, QByteArray data)
                     {
                         Q_UNUSED(id);
                         Q_UNUSED(len);
                         QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
                         if (jsonDoc.isNull())
                         {
                             return;
                         }

                         QJsonObject jsonObj = jsonDoc.object();
                         if (!jsonObj.contains("error"))
                         {
                             return;
                         }

                         emit sig_message_status(jsonObj);
                     });

    _handlers.insert(ID_NOTIFY_MSG_STATUS_REQ,
                     [this](ReqId id, int len, QByteArray data)
                     {
                         Q_UNUSED(id);
                         Q_UNUSED(len);
                         QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
                         if (jsonDoc.isNull() || !jsonDoc.isObject())
                         {
                             return;
                         }
                         emit sig_message_status(jsonDoc.object());
                     });

    _handlers.insert(ID_NOTIFY_TEXT_CHAT_MSG_REQ,
                     [this](ReqId id, int len, QByteArray data)
                     {
                         Q_UNUSED(len);
                         qDebug() << "handle id is " << id << " data is " << data;

                         QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
                         if (jsonDoc.isNull())
                         {
                             return;
                         }

                         QJsonObject jsonObj = jsonDoc.object();
                         if (jsonObj.value("error").toInt(ErrorCodes::ERR_JSON) != ErrorCodes::SUCCESS)
                         {
                             return;
                         }

                         auto msg_ptr = std::make_shared<TextChatMsg>(jsonObj["fromuid"].toInt(),
                                                                      jsonObj["touid"].toInt(),
                                                                      jsonObj["text_array"].toArray());
                         emit sig_text_chat_msg(msg_ptr);
                     });

    _handlers.insert(ID_EDIT_PRIVATE_MSG_RSP,
                     [this](ReqId id, int len, QByteArray data)
                     {
                         Q_UNUSED(len);
                         QJsonObject jsonObj;
                         parseGroupResponse(id, data, jsonObj);
                     });
    _handlers.insert(ID_REVOKE_PRIVATE_MSG_RSP,
                     [this](ReqId id, int len, QByteArray data)
                     {
                         Q_UNUSED(len);
                         QJsonObject jsonObj;
                         parseGroupResponse(id, data, jsonObj);
                     });
    _handlers.insert(ID_FORWARD_PRIVATE_MSG_RSP,
                     [this](ReqId id, int len, QByteArray data)
                     {
                         Q_UNUSED(len);
                         QJsonObject jsonObj;
                         parseGroupResponse(id, data, jsonObj);
                     });

    _handlers.insert(ID_PRIVATE_HISTORY_RSP,
                     [this](ReqId id, int len, QByteArray data)
                     {
                         Q_UNUSED(len);
                         Q_UNUSED(id);
                         QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
                         if (jsonDoc.isNull() || !jsonDoc.isObject())
                         {
                             QJsonObject fallback;
                             fallback["error"] = ErrorCodes::ERR_JSON;
                             emit sig_private_history_rsp(fallback);
                             return;
                         }
                         emit sig_private_history_rsp(jsonDoc.object());
                     });

    _handlers.insert(ID_NOTIFY_PRIVATE_MSG_CHANGED_REQ,
                     [this](ReqId id, int len, QByteArray data)
                     {
                         Q_UNUSED(id);
                         Q_UNUSED(len);
                         QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
                         if (jsonDoc.isNull() || !jsonDoc.isObject())
                         {
                             return;
                         }
                         const QJsonObject jsonObj = jsonDoc.object();
                         if (jsonObj.value("error").toInt(ErrorCodes::ERR_JSON) != ErrorCodes::SUCCESS)
                         {
                             return;
                         }
                         emit sig_private_msg_changed(jsonObj);
                     });

    _handlers.insert(ID_NOTIFY_PRIVATE_READ_ACK_REQ,
                     [this](ReqId id, int len, QByteArray data)
                     {
                         Q_UNUSED(id);
                         Q_UNUSED(len);
                         QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
                         if (jsonDoc.isNull() || !jsonDoc.isObject())
                         {
                             return;
                         }
                         const QJsonObject jsonObj = jsonDoc.object();
                         if (jsonObj.value("error").toInt(ErrorCodes::ERR_JSON) != ErrorCodes::SUCCESS)
                         {
                             return;
                         }
                         emit sig_private_read_ack(jsonObj);
                     });
}
