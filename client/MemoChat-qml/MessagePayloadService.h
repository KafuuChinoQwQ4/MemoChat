#ifndef MESSAGEPAYLOADSERVICE_H
#define MESSAGEPAYLOADSERVICE_H

#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <memory>
#include "userdata.h"

struct MessageUpdateFields
{
    QString msgId;
    QString content;
    QString state;
    qint64 editedAtMs = 0;
    qint64 deletedAtMs = 0;
};

class MessagePayloadService
{
public:
    static qint64 normalizeEpochMs(qint64 value, bool useNowWhenEmpty = false);
    static QString extractForwardMetaJson(const QJsonValue &value);
    static QString resolveMessageState(const QString &content,
                                       qint64 editedAtMs,
                                       qint64 deletedAtMs,
                                       const QString &msgType = QString());
    static MessageUpdateFields parseMessageUpdateFields(const QJsonObject &payload,
                                                        const QString &event = QString(),
                                                        bool allowEmptyContent = false);
    static std::shared_ptr<TextChatData> buildPrivateHistoryMessage(const QJsonObject &obj);
    static std::shared_ptr<TextChatData> buildPrivateForwardedMessage(const QJsonObject &payload,
                                                                      const QJsonObject &msgObj,
                                                                      int fromUid);
    static std::shared_ptr<TextChatData> buildGroupAckMessage(const QJsonObject &payload,
                                                              const QJsonObject &msgObj,
                                                              int fromUid,
                                                              const QString &fromName,
                                                              const QString &fromIcon);
    static std::shared_ptr<TextChatData> buildGroupIncomingMessage(const GroupChatMsg &msg);
};

#endif // MESSAGEPAYLOADSERVICE_H
