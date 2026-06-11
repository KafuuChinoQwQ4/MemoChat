#ifndef INCOMINGMESSAGEROUTER_H
#define INCOMINGMESSAGEROUTER_H

#include "UserMessageData.h"

#include <QSet>
#include <QString>
#include <QVector>
#include <QtGlobal>

#include <functional>
#include <memory>

struct IncomingMessageRouterReadiness
{
    bool onChatPage = false;
    bool dialogsReady = false;
    bool chatListInitialized = false;
};

struct IncomingMessageRouterDispatch
{
    std::function<void(std::shared_ptr<TextChatMsg>)> applyPrivateMessage;
    std::function<void(std::shared_ptr<GroupChatMsg>)> applyGroupMessage;
};

struct IncomingMessageRouteResult
{
    bool accepted = false;
    bool buffered = false;
    bool applied = false;
    int pendingCount = 0;
};

struct PendingIncomingMessage
{
    enum class Kind
    {
        Private,
        Group
    };

    Kind kind = Kind::Private;
    std::shared_ptr<TextChatMsg> privateMsg;
    std::shared_ptr<GroupChatMsg> groupMsg;
    QString key;
    qint64 createdAt = 0;
    qint64 sequence = 0;
};

class IncomingMessageRouter
{
public:
    bool shouldBuffer(const IncomingMessageRouterReadiness& readiness) const;
    IncomingMessageRouteResult routePrivateMessage(const IncomingMessageRouterReadiness& readiness,
                                                   std::shared_ptr<TextChatMsg> message,
                                                   const IncomingMessageRouterDispatch& dispatch);
    IncomingMessageRouteResult routeGroupMessage(const IncomingMessageRouterReadiness& readiness,
                                                 std::shared_ptr<GroupChatMsg> message,
                                                 const IncomingMessageRouterDispatch& dispatch);
    bool bufferPrivateMessage(const IncomingMessageRouterReadiness& readiness, std::shared_ptr<TextChatMsg> message);
    bool bufferGroupMessage(const IncomingMessageRouterReadiness& readiness, std::shared_ptr<GroupChatMsg> message);
    int flush(const IncomingMessageRouterReadiness& readiness, const IncomingMessageRouterDispatch& dispatch);
    void clear();
    int pendingCount() const;

    static constexpr qsizetype maxMessages = 1000;

private:
    bool append(PendingIncomingMessage pending);

    QVector<PendingIncomingMessage> _messages;
    QSet<QString> _keys;
    qint64 _nextSequence = 0;
};

#endif // INCOMINGMESSAGEROUTER_H
