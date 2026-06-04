#include "AppController.h"

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
        return QString();
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
        return QString();
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

bool AppController::shouldBufferIncomingMessages() const
{
    return _page == ChatPage && (!_bootstrap_state.dialogsReady || !_bootstrap_state.chatListInitialized);
}

bool AppController::bufferIncomingPrivateMessage(std::shared_ptr<TextChatMsg> msg)
{
    if (!shouldBufferIncomingMessages() || !msg || msg->_chat_msgs.empty())
    {
        return false;
    }

    const QString key = privateMessageKey(msg);
    if (key.isEmpty() || _incoming_buffer_state.keys.contains(key))
    {
        return true;
    }
    if (_incoming_buffer_state.messages.size() >= _incoming_buffer_state.maxMessages)
    {
        const PendingIncomingMessage dropped = _incoming_buffer_state.messages.takeFirst();
        _incoming_buffer_state.keys.remove(dropped.key);
        qWarning() << "Incoming message buffer full, dropped oldest private/group push";
    }

    PendingIncomingMessage pending;
    pending.kind = PendingIncomingMessage::Kind::Private;
    pending.privateMsg = std::move(msg);
    pending.key = key;
    pending.createdAt = firstPrivateCreatedAt(pending.privateMsg);
    pending.sequence = _incoming_buffer_state.nextSequence++;
    _incoming_buffer_state.keys.insert(pending.key);
    _incoming_buffer_state.messages.push_back(std::move(pending));
    return true;
}

bool AppController::bufferIncomingGroupMessage(std::shared_ptr<GroupChatMsg> msg)
{
    if (!shouldBufferIncomingMessages() || !msg || !msg->_msg)
    {
        return false;
    }

    const QString key = groupMessageKey(msg);
    if (key.isEmpty() || _incoming_buffer_state.keys.contains(key))
    {
        return true;
    }
    if (_incoming_buffer_state.messages.size() >= _incoming_buffer_state.maxMessages)
    {
        const PendingIncomingMessage dropped = _incoming_buffer_state.messages.takeFirst();
        _incoming_buffer_state.keys.remove(dropped.key);
        qWarning() << "Incoming message buffer full, dropped oldest private/group push";
    }

    PendingIncomingMessage pending;
    pending.kind = PendingIncomingMessage::Kind::Group;
    pending.groupMsg = std::move(msg);
    pending.key = key;
    pending.createdAt = pending.groupMsg->_msg->_created_at;
    pending.sequence = _incoming_buffer_state.nextSequence++;
    _incoming_buffer_state.keys.insert(pending.key);
    _incoming_buffer_state.messages.push_back(std::move(pending));
    return true;
}

void AppController::flushPendingIncomingMessages()
{
    if (shouldBufferIncomingMessages() || _incoming_buffer_state.messages.isEmpty())
    {
        return;
    }

    auto pending = _incoming_buffer_state.messages;
    clearPendingIncomingMessages();
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
    for (const auto& one : pending)
    {
        if (one.kind == PendingIncomingMessage::Kind::Private)
        {
            applyTextChatMsg(one.privateMsg);
        }
        else
        {
            applyGroupChatMsg(one.groupMsg);
        }
    }
}

void AppController::clearPendingIncomingMessages()
{
    _incoming_buffer_state.clear();
}
