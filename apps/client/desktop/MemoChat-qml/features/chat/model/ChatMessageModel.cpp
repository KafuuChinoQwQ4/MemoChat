#include "ChatMessageModel.h"

namespace
{
constexpr qint64 kAvatarGroupWindowMs = 60000;
}

ChatMessageModel::ChatMessageModel(QObject* parent)
    : QAbstractListModel(parent)
{
    _time_divider_refresh_timer.setSingleShot(true);
    connect(&_time_divider_refresh_timer,
            &QTimer::timeout,
            this,
            [this]()
            {
                if (_items.empty())
                {
                    return;
                }
                recomputeAvatarFlags();
                const QModelIndex top = index(0, 0);
                const QModelIndex bottom = index(rowCount() - 1, 0);
                emit dataChanged(top, bottom, {ShowTimeDividerRole, TimeDividerTextRole, ShowAvatarRole});
                restartTimeDividerRefreshTimer();
            });
}

bool ChatMessageModel::lessThan(const MessageEntry& lhs, const MessageEntry& rhs)
{
    if (lhs.groupSeq > 0 && rhs.groupSeq > 0 && lhs.groupSeq != rhs.groupSeq)
    {
        return lhs.groupSeq < rhs.groupSeq;
    }
    if (lhs.createdAt != rhs.createdAt)
    {
        return lhs.createdAt < rhs.createdAt;
    }
    if (lhs.serverMsgId > 0 && rhs.serverMsgId > 0 && lhs.serverMsgId != rhs.serverMsgId)
    {
        return lhs.serverMsgId < rhs.serverMsgId;
    }
    return lhs.msgId < rhs.msgId;
}

bool ChatMessageModel::shouldShowAvatarForEntry(const MessageEntry* previous, const MessageEntry& current)
{
    if (!previous)
    {
        return true;
    }
    if (current.fromUid != previous->fromUid)
    {
        return true;
    }
    if (current.createdAt <= 0 || previous->createdAt <= 0)
    {
        return true;
    }
    return (current.createdAt - previous->createdAt) > kAvatarGroupWindowMs;
}

int ChatMessageModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
    {
        return 0;
    }

    return static_cast<int>(_items.size());
}

QVariant ChatMessageModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount())
    {
        return {};
    }

    const MessageEntry& entry = _items[static_cast<size_t>(index.row())];
    switch (role)
    {
        case MsgIdRole:
            return entry.msgId;
        case ContentRole:
            return entry.content;
        case FromUidRole:
            return entry.fromUid;
        case ToUidRole:
            return entry.toUid;
        case OutgoingRole:
            return entry.outgoing;
        case MsgTypeRole:
            return entry.msgType;
        case FileNameRole:
            return entry.fileName;
        case RawContentRole:
            return entry.rawContent;
        case SenderNameRole:
            return entry.senderName;
        case SenderIconRole:
            return entry.senderIcon;
        case ShowAvatarRole:
            return entry.showAvatar;
        case CreatedAtRole:
            return entry.createdAt;
        case ShowTimeDividerRole:
            return shouldShowTimeDivider(index.row());
        case TimeDividerTextRole:
            return timeDividerText(index.row());
        case MessageStateRole:
            return entry.messageState;
        case IsReplyRole:
            return entry.isReply;
        case ReplyToMsgIdRole:
            return entry.replyToMsgId;
        case ReplySenderRole:
            return entry.replySender;
        case ReplyPreviewRole:
            return entry.replyPreview;
        case ReplyToServerMsgIdRole:
            return entry.replyToServerMsgId;
        case ForwardMetaRole:
            return entry.forwardMetaJson;
        case EditedAtMsRole:
            return entry.editedAtMs;
        case DeletedAtMsRole:
            return entry.deletedAtMs;
        default:
            return {};
    }
}

QHash<int, QByteArray> ChatMessageModel::roleNames() const
{
    return {{MsgIdRole, "msgId"},
            {ContentRole, "content"},
            {FromUidRole, "fromUid"},
            {ToUidRole, "toUid"},
            {OutgoingRole, "outgoing"},
            {MsgTypeRole, "msgType"},
            {FileNameRole, "fileName"},
            {RawContentRole, "rawContent"},
            {SenderNameRole, "senderName"},
            {SenderIconRole, "senderIcon"},
            {ShowAvatarRole, "showAvatar"},
            {CreatedAtRole, "createdAt"},
            {ShowTimeDividerRole, "showTimeDivider"},
            {TimeDividerTextRole, "timeDividerText"},
            {MessageStateRole, "messageState"},
            {IsReplyRole, "isReply"},
            {ReplyToMsgIdRole, "replyToMsgId"},
            {ReplySenderRole, "replySender"},
            {ReplyPreviewRole, "replyPreview"},
            {ReplyToServerMsgIdRole, "replyToServerMsgId"},
            {ForwardMetaRole, "forwardMeta"},
            {EditedAtMsRole, "editedAtMs"},
            {DeletedAtMsRole, "deletedAtMs"}};
}
