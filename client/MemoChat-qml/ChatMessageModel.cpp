#include "ChatMessageModel.h"
#include "MessageContentCodec.h"
#include <limits>
#include <algorithm>
#include <unordered_set>

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
    case SenderNameRole:
        return entry.senderName;
    case SenderIconRole:
        return entry.senderIcon;
    case ShowAvatarRole:
        return entry.showAvatar;
    case CreatedAtRole:
        return entry.createdAt;
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
        {SenderNameRole, "senderName"},
        {SenderIconRole, "senderIcon"},
        {ShowAvatarRole, "showAvatar"},
        {CreatedAtRole, "createdAt"}
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

    std::vector<MessageEntry> incoming;
    incoming.reserve(messages.size());
    std::unordered_set<std::string> seen;
    seen.reserve(messages.size());
    for (const auto &message : messages) {
        if (!message) {
            continue;
        }
        MessageEntry entry = toEntry(message, selfUid);
        const std::string key = entry.msgId.toStdString();
        if (key.empty() || seen.find(key) != seen.end()) {
            continue;
        }
        seen.insert(key);
        incoming.push_back(std::move(entry));
    }

    std::sort(incoming.begin(), incoming.end(),
              [](const MessageEntry &lhs, const MessageEntry &rhs) {
                  if (lhs.createdAt != rhs.createdAt) {
                      return lhs.createdAt < rhs.createdAt;
                  }
                  return lhs.msgId < rhs.msgId;
              });
    _items = std::move(incoming);
    int previousSenderUid = std::numeric_limits<int>::min();
    for (auto &entry : _items) {
        entry.showAvatar = (entry.fromUid != previousSenderUid);
        previousSenderUid = entry.fromUid;
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
    if (containsMessage(entry.msgId)) {
        return;
    }
    int insertPos = rowCount();
    auto insertIt = _items.end();
    if (!_items.empty()) {
        insertIt = std::upper_bound(
            _items.begin(), _items.end(), entry,
            [](const MessageEntry &lhs, const MessageEntry &rhs) {
                if (lhs.createdAt != rhs.createdAt) {
                    return lhs.createdAt < rhs.createdAt;
                }
                return lhs.msgId < rhs.msgId;
            });
        insertPos = static_cast<int>(std::distance(_items.begin(), insertIt));
    }

    beginInsertRows(QModelIndex(), insertPos, insertPos);
    _items.insert(insertIt, entry);
    endInsertRows();
    refreshAvatarFlags();
    emit countChanged();
}

void ChatMessageModel::upsertMessage(const std::shared_ptr<TextChatData> &message, int selfUid)
{
    if (!message) {
        return;
    }

    beginResetModel();
    MessageEntry entry = toEntry(message, selfUid);
    bool found = false;
    for (auto &item : _items) {
        if (item.msgId == entry.msgId) {
            item = entry;
            found = true;
            break;
        }
    }
    if (!found) {
        _items.push_back(std::move(entry));
    }

    std::sort(_items.begin(), _items.end(),
              [](const MessageEntry &lhs, const MessageEntry &rhs) {
                  if (lhs.createdAt != rhs.createdAt) {
                      return lhs.createdAt < rhs.createdAt;
                  }
                  return lhs.msgId < rhs.msgId;
              });

    if (!_items.empty()) {
        int previousSenderUid = std::numeric_limits<int>::min();
        for (auto &item : _items) {
            item.showAvatar = (item.fromUid != previousSenderUid);
            previousSenderUid = item.fromUid;
        }
    }
    endResetModel();
    emit countChanged();
}

void ChatMessageModel::prependMessages(const std::vector<std::shared_ptr<TextChatData> > &messages, int selfUid)
{
    if (messages.empty()) {
        return;
    }

    std::vector<MessageEntry> incoming;
    incoming.reserve(messages.size());
    for (const auto &message : messages) {
        if (!message) {
            continue;
        }
        MessageEntry entry = toEntry(message, selfUid);
        if (containsMessage(entry.msgId)) {
            continue;
        }
        incoming.push_back(std::move(entry));
    }
    if (incoming.empty()) {
        return;
    }

    std::sort(incoming.begin(), incoming.end(),
              [](const MessageEntry &lhs, const MessageEntry &rhs) {
                  if (lhs.createdAt != rhs.createdAt) {
                      return lhs.createdAt < rhs.createdAt;
                  }
                  return lhs.msgId < rhs.msgId;
              });

    beginInsertRows(QModelIndex(), 0, static_cast<int>(incoming.size()) - 1);
    _items.insert(_items.begin(), incoming.begin(), incoming.end());
    endInsertRows();
    refreshAvatarFlags();
    emit countChanged();
}

qint64 ChatMessageModel::earliestCreatedAt() const
{
    if (_items.empty()) {
        return 0;
    }
    return _items.front().createdAt;
}

bool ChatMessageModel::containsMessage(const QString &msgId) const
{
    if (msgId.isEmpty()) {
        return false;
    }
    return std::any_of(_items.begin(), _items.end(),
                       [&msgId](const MessageEntry &entry) { return entry.msgId == msgId; });
}

ChatMessageModel::MessageEntry ChatMessageModel::toEntry(const std::shared_ptr<TextChatData> &message, int selfUid) const
{
    MessageEntry entry;
    entry.msgId = message->_msg_id;
    const DecodedMessageContent decoded = MessageContentCodec::decode(message->_msg_content);
    entry.content = decoded.content;
    entry.msgType = decoded.type;
    entry.fileName = decoded.fileName;
    entry.senderName = message->_from_name;
    entry.senderIcon = message->_from_icon;
    entry.fromUid = message->_from_uid;
    entry.toUid = message->_to_uid;
    entry.outgoing = (message->_from_uid == selfUid);
    entry.showAvatar = true;
    entry.createdAt = message->_created_at;
    return entry;
}

void ChatMessageModel::refreshAvatarFlags()
{
    int previousSenderUid = std::numeric_limits<int>::min();
    for (auto &entry : _items) {
        entry.showAvatar = (entry.fromUid != previousSenderUid);
        previousSenderUid = entry.fromUid;
    }
    if (!_items.empty()) {
        const QModelIndex top = index(0, 0);
        const QModelIndex bottom = index(rowCount() - 1, 0);
        emit dataChanged(top, bottom, {ShowAvatarRole});
    }
}
