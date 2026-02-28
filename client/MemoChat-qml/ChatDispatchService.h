#ifndef CHATDISPATCHSERVICE_H
#define CHATDISPATCHSERVICE_H

#include <QString>
#include <QByteArray>
#include <functional>

struct OutgoingChatPacket {
    int fromUid = 0;
    int toUid = 0;
    QString msgId;
    QString content;
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
