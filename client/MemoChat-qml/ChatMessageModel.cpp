#include "ChatMessageModel.h"
#include "MessageContentCodec.h"
#include <limits>

ChatMessageModel::ChatMessageModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int ChatMessageModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return static_cast<int>(_items.size());
}

QVariant ChatMessageModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return {};
    }

    const MessageEntry &entry = _items[static_cast<size_t>(index.row())];
    switch (role) {
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
    case ShowAvatarRole:
        return entry.showAvatar;
    default:
        return {};
    }
}

QHash<int, QByteArray> ChatMessageModel::roleNames() const
{
    return {
        {MsgIdRole, "msgId"},
        {ContentRole, "content"},
        {FromUidRole, "fromUid"},
        {ToUidRole, "toUid"},
        {OutgoingRole, "outgoing"},
        {MsgTypeRole, "msgType"},
        {FileNameRole, "fileName"},
        {ShowAvatarRole, "showAvatar"}
    };
}

void ChatMessageModel::clear()
{
    if (_items.empty()) {
        return;
    }

    beginResetModel();
    _items.clear();
    endResetModel();
    emit countChanged();
}

void ChatMessageModel::setMessages(const std::vector<std::shared_ptr<TextChatData> > &messages, int selfUid)
{
    beginResetModel();
    _items.clear();
    _items.reserve(messages.size());

    int previousSenderUid = std::numeric_limits<int>::min();
    for (const auto &message : messages) {
        if (!message) {
            continue;
        }
        MessageEntry entry = toEntry(message, selfUid);
        entry.showAvatar = (entry.fromUid != previousSenderUid);
        previousSenderUid = entry.fromUid;
        _items.push_back(std::move(entry));
    }

    endResetModel();
    emit countChanged();
}

void ChatMessageModel::appendMessage(const std::shared_ptr<TextChatData> &message, int selfUid)
{
    if (!message) {
        return;
    }

    MessageEntry entry = toEntry(message, selfUid);
    entry.showAvatar = _items.empty() || _items.back().fromUid != entry.fromUid;

    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    _items.push_back(entry);
    endInsertRows();
    emit countChanged();
}

ChatMessageModel::MessageEntry ChatMessageModel::toEntry(const std::shared_ptr<TextChatData> &message, int selfUid) const
{
    MessageEntry entry;
    entry.msgId = message->_msg_id;
    const DecodedMessageContent decoded = MessageContentCodec::decode(message->_msg_content);
    entry.content = decoded.content;
    entry.msgType = decoded.type;
    entry.fileName = decoded.fileName;
    entry.fromUid = message->_from_uid;
    entry.toUid = message->_to_uid;
    entry.outgoing = (message->_from_uid == selfUid);
    entry.showAvatar = true;
    return entry;
}
