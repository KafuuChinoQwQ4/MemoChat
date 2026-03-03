#ifndef CHATDISPATCHSERVICE_H
#define CHATDISPATCHSERVICE_H

#include <QString>
#include <QByteArray>
#include <QtGlobal>
#include <functional>

struct OutgoingChatPacket {
    int fromUid = 0;
    int toUid = 0;
    QString msgId;
    QString content;
    qint64 createdAt = 0;
};

class ChatDispatchService
{
public:
    using SendPayloadFunc = std::function<void(const QByteArray &)>;

    static QByteArray buildTextPayload(const OutgoingChatPacket &packet);
    static bool dispatchTextPayload(const OutgoingChatPacket &packet,
                                    const SendPayloadFunc &sendPayload,
                                    QString *errorText = nullptr);
};

#endif // CHATDISPATCHSERVICE_H
