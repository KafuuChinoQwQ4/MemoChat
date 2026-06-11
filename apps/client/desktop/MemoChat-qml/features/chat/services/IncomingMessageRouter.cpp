#include "IncomingMessageRouter.h"

#include <QDebug>
#include <QStringList>

#include <algorithm>
#include <utility>

namespace
{
qint64 firstPrivateCreatedAt(const std::shared_ptr<TextChatMsg>& msg)
{
    qint64 createdAt = 0;
    if (!msg)
    {
        return createdAt;
    }
    for (const auto& one : msg->_chat_msgs)
    {
        if (one && (createdAt <= 0 || one->_created_at < createdAt))
        {
            createdAt = one->_created_at;
        }
    }
    return createdAt;
}

QString privateMessageKey(const std::shared_ptr<TextChatMsg>& msg)
{
    if (!msg)
    {
        return {};
    }

    QStringList parts;
    parts.reserve(static_cast<qsizetype>(msg->_chat_msgs.size()) + 3);
    parts << QStringLiteral("private") << QString::number(msg->_from_uid) << QString::number(msg->_to_uid);
    for (const auto& one : msg->_chat_msgs)
    {
        if (!one)
        {
            continue;
        }
        if (!one->_msg_id.isEmpty())
        {
            parts << one->_msg_id;
        }
        else
        {
            parts << QString::number(one->_created_at) << one->_msg_content;
        }
    }
    return parts.join(QLatin1Char(':'));
}

QString groupMessageKey(const std::shared_ptr<GroupChatMsg>& msg)
{
    if (!msg || !msg->_msg)
    {
        return {};
    }
    if (!msg->_msg->_msg_id.isEmpty())
    {
        return QStringLiteral("group:%1:%2").arg(msg->_group_id).arg(msg->_msg->_msg_id);
    }
    return QStringLiteral("group:%1:%2:%3")
                              .arg(msg->_group_id)
                              .arg(msg->_msg->_created_at)
                              .arg(msg->_msg->_msg_content);
}
} // namespace

bool IncomingMessageRouter::shouldBuffer(const IncomingMessageRouterReadiness& readiness) const
{
    return readiness.onChatPage && (!readiness.dialogsReady || !readiness.chatListInitialized);
}

IncomingMessageRouteResult IncomingMessageRouter::routePrivateMessage(const IncomingMessageRouterReadiness& readiness,
                                                                      std::shared_ptr<TextChatMsg> message,
                                                                      const IncomingMessageRouterDispatch& dispatch)
{
    IncomingMessageRouteResult result;
    if (!message || message->_chat_msgs.empty())
    {
        result.pendingCount = pendingCount();
        return result;
    }
    result.accepted = true;
    if (bufferPrivateMessage(readiness, message))
    {
        result.buffered = true;
        result.pendingCount = pendingCount();
        return result;
    }
    if (dispatch.applyPrivateMessage)
    {
        dispatch.applyPrivateMessage(std::move(message));
        result.applied = true;
    }
    result.pendingCount = pendingCount();
    return result;
}

IncomingMessageRouteResult IncomingMessageRouter::routeGroupMessage(const IncomingMessageRouterReadiness& readiness,
                                                                    std::shared_ptr<GroupChatMsg> message,
                                                                    const IncomingMessageRouterDispatch& dispatch)
{
    IncomingMessageRouteResult result;
    if (!message || !message->_msg)
    {
        result.pendingCount = pendingCount();
        return result;
    }
    result.accepted = true;
    if (bufferGroupMessage(readiness, message))
    {
        result.buffered = true;
        result.pendingCount = pendingCount();
        return result;
    }
    if (dispatch.applyGroupMessage)
    {
        dispatch.applyGroupMessage(std::move(message));
        result.applied = true;
    }
    result.pendingCount = pendingCount();
    return result;
}

bool IncomingMessageRouter::bufferPrivateMessage(const IncomingMessageRouterReadiness& readiness,
                                                 std::shared_ptr<TextChatMsg> message)
{
    if (!shouldBuffer(readiness) || !message || message->_chat_msgs.empty())
    {
        return false;
    }

    PendingIncomingMessage pending;
    pending.kind = PendingIncomingMessage::Kind::Private;
    pending.privateMsg = std::move(message);
    pending.key = privateMessageKey(pending.privateMsg);
    pending.createdAt = firstPrivateCreatedAt(pending.privateMsg);
    return append(std::move(pending));
}

bool IncomingMessageRouter::bufferGroupMessage(const IncomingMessageRouterReadiness& readiness,
                                               std::shared_ptr<GroupChatMsg> message)
{
    if (!shouldBuffer(readiness) || !message || !message->_msg)
    {
        return false;
    }

    PendingIncomingMessage pending;
    pending.kind = PendingIncomingMessage::Kind::Group;
    pending.groupMsg = std::move(message);
    pending.key = groupMessageKey(pending.groupMsg);
    pending.createdAt = pending.groupMsg->_msg->_created_at;
    return append(std::move(pending));
}

int IncomingMessageRouter::flush(const IncomingMessageRouterReadiness& readiness,
                                 const IncomingMessageRouterDispatch& dispatch)
{
    if (shouldBuffer(readiness) || _messages.isEmpty())
    {
        return 0;
    }

    auto pending = _messages;
    clear();
    std::stable_sort(pending.begin(),
                     pending.end(),
                     [](const PendingIncomingMessage& lhs, const PendingIncomingMessage& rhs)
                     {
                         if (lhs.createdAt != rhs.createdAt)
                         {
                             return lhs.createdAt < rhs.createdAt;
                         }
                         return lhs.sequence < rhs.sequence;
                     });

    int applied = 0;
    for (const auto& one : pending)
    {
        if (one.kind == PendingIncomingMessage::Kind::Private)
        {
            if (dispatch.applyPrivateMessage)
            {
                dispatch.applyPrivateMessage(one.privateMsg);
                ++applied;
            }
        }
        else if (dispatch.applyGroupMessage)
        {
            dispatch.applyGroupMessage(one.groupMsg);
            ++applied;
        }
    }
    return applied;
}

void IncomingMessageRouter::clear()
{
    _messages.clear();
    _keys.clear();
    _nextSequence = 0;
}

int IncomingMessageRouter::pendingCount() const
{
    return _messages.size();
}

bool IncomingMessageRouter::append(PendingIncomingMessage pending)
{
    if (pending.key.isEmpty() || _keys.contains(pending.key))
    {
        return true;
    }
    if (_messages.size() >= maxMessages)
    {
        const PendingIncomingMessage dropped = _messages.takeFirst();
        _keys.remove(dropped.key);
        qWarning() << "Incoming message buffer full, dropped oldest private/group push";
    }

    pending.sequence = _nextSequence++;
    _keys.insert(pending.key);
    _messages.push_back(std::move(pending));
    return true;
}
