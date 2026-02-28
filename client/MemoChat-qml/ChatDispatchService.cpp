#include "ChatDispatchService.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

QByteArray ChatDispatchService::buildTextPayload(const OutgoingChatPacket &packet)
{
    QJsonObject msgObj;
    msgObj["content"] = packet.content;
    msgObj["msgid"] = packet.msgId;

    QJsonArray textArray;
    textArray.append(msgObj);

    QJsonObject payload;
    payload["fromuid"] = packet.fromUid;
    payload["touid"] = packet.toUid;
    payload["text_array"] = textArray;
    return QJsonDocument(payload).toJson(QJsonDocument::Compact);
}

bool ChatDispatchService::dispatchTextPayload(const OutgoingChatPacket &packet,
                                              const SendPayloadFunc &sendPayload,
                                              QString *errorText)
{
    if (packet.fromUid <= 0 || packet.toUid <= 0) {
        if (errorText) {
            *errorText = "消息参数非法";
        }
        return false;
    }
    if (packet.msgId.isEmpty() || packet.content.isEmpty()) {
        if (errorText) {
            *errorText = "消息内容不能为空";
        }
        return false;
    }
    if (!sendPayload) {
        if (errorText) {
            *errorText = "消息发送器不可用";
        }
        return false;
    }

    sendPayload(buildTextPayload(packet));
    return true;
}
