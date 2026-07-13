#include "ChatMessageModel.h"

#include "MessageContentCodec.h"

#include <QDateTime>
#include <algorithm>
#include <unordered_set>

void ChatMessageModel::clear()
{
    if (_items.empty())
    {
        return;
    }

    beginResetModel();
    _items.clear();
    endResetModel();
    stopTimeDividerRefreshTimer();
    emit countChanged();
}

void ChatMessageModel::setMessages(const std::vector<std::shared_ptr<TextChatData>>& messages, int selfUid)
{
    setMessagesAtomic(messages, selfUid);
}

void ChatMessageModel::setMessagesAtomic(const std::vector<std::shared_ptr<TextChatData>>& messages, int selfUid)
{
    const qint64 startTs = QDateTime::currentMSecsSinceEpoch();

    _itemsBuffer.clear();
    _itemsBuffer.reserve(messages.size());

    std::unordered_set<std::string> seen;
    seen.reserve(messages.size());
    for (const auto& message : messages)
    {
        if (!message)
        {
            continue;
        }
        MessageEntry entry = toEntry(message, selfUid);
        const std::string key = entry.msgId.toStdString();
        if (key.empty() || seen.find(key) != seen.end())
        {
            continue;
        }
        seen.insert(key);
        _itemsBuffer.push_back(std::move(entry));
    }

    const qint64 afterBuildTs = QDateTime::currentMSecsSinceEpoch();

    std::sort(_itemsBuffer.begin(), _itemsBuffer.end(), lessThan);

    const qint64 afterSortTs = QDateTime::currentMSecsSinceEpoch();

    // 原子切换：先触发 reset，切换缓冲区，再 end reset
    beginResetModel();
    _items = std::move(_itemsBuffer);
    _itemsBuffer.clear();
    recomputeAvatarFlags();
    endResetModel();

    const qint64 endTs = QDateTime::currentMSecsSinceEpoch();

    qInfo() << "[PERF] ChatMessageModel::setMessagesAtomic - messages:" << messages.size()
            << "| build:" << (afterBuildTs - startTs) << "ms"
            << "| sort:" << (afterSortTs - afterBuildTs) << "ms"
            << "| ui:" << (endTs - afterSortTs) << "ms"
            << "| total:" << (endTs - startTs) << "ms";

    restartTimeDividerRefreshTimer();
    emit countChanged();
}

void ChatMessageModel::appendMessage(const std::shared_ptr<TextChatData>& message, int selfUid)
{
    if (!message)
    {
        return;
    }

    MessageEntry entry = toEntry(message, selfUid);
    if (indexOfMessage(entry.msgId) >= 0)
    {
        return;
    }
    const int insertPos = findInsertPosition(entry);
    auto insertIt = _items.begin() + insertPos;

    beginInsertRows(QModelIndex(), insertPos, insertPos);
    _items.insert(insertIt, entry);
    endInsertRows();
    recomputeAvatarFlags();
    refreshAvatarRange(insertPos - 1, insertPos + 1);
    refreshTimeDividerRange(insertPos - 1, insertPos + 1);
    restartTimeDividerRefreshTimer();
    emit countChanged();
}

void ChatMessageModel::upsertMessage(const std::shared_ptr<TextChatData>& message, int selfUid)
{
    if (!message)
    {
        return;
    }

    MessageEntry entry = toEntry(message, selfUid);
    const int existingIndex = indexOfMessage(entry.msgId);
    if (existingIndex < 0)
    {
        appendMessage(message, selfUid);
        return;
    }

    const MessageEntry existing = _items[static_cast<size_t>(existingIndex)];
    const bool orderUnchanged = (existing.groupSeq == entry.groupSeq) && (existing.createdAt == entry.createdAt) &&
                                (existing.serverMsgId == entry.serverMsgId);
    if (orderUnchanged)
    {
        _items[static_cast<size_t>(existingIndex)] = entry;
        recomputeAvatarFlags();
        refreshSurroundingRows(
            existingIndex,
            {ContentRole,         RawContentRole,         OutgoingRole,    MsgTypeRole,      FileNameRole,
             SenderNameRole,      SenderIconRole,         ShowAvatarRole,  CreatedAtRole,    ShowTimeDividerRole,
             TimeDividerTextRole, MessageStateRole,       IsReplyRole,     ReplyToMsgIdRole, ReplySenderRole,
             ReplyPreviewRole,    ReplyToServerMsgIdRole, ForwardMetaRole, EditedAtMsRole,   DeletedAtMsRole});
        restartTimeDividerRefreshTimer();
        return;
    }

    beginRemoveRows(QModelIndex(), existingIndex, existingIndex);
    _items.erase(_items.begin() + existingIndex);
    endRemoveRows();

    const int insertPos = findInsertPosition(entry);
    beginInsertRows(QModelIndex(), insertPos, insertPos);
    _items.insert(_items.begin() + insertPos, entry);
    endInsertRows();
    recomputeAvatarFlags();
    refreshAvatarRange(qMin(existingIndex, insertPos) - 1, qMax(existingIndex, insertPos) + 1);
    refreshTimeDividerRange(qMin(existingIndex, insertPos) - 1, qMax(existingIndex, insertPos) + 1);
    restartTimeDividerRefreshTimer();
}

void ChatMessageModel::prependMessages(const std::vector<std::shared_ptr<TextChatData>>& messages, int selfUid)
{
    if (messages.empty())
    {
        return;
    }

    std::vector<MessageEntry> incoming;
    incoming.reserve(messages.size());
    for (const auto& message : messages)
    {
        if (!message)
        {
            continue;
        }
        MessageEntry entry = toEntry(message, selfUid);
        if (indexOfMessage(entry.msgId) >= 0)
        {
            continue;
        }
        incoming.push_back(std::move(entry));
    }
    if (incoming.empty())
    {
        return;
    }

    std::sort(incoming.begin(), incoming.end(), lessThan);

    beginInsertRows(QModelIndex(), 0, static_cast<int>(incoming.size()) - 1);
    _items.insert(_items.begin(), incoming.begin(), incoming.end());
    endInsertRows();
    recomputeAvatarFlags();
    refreshAvatarRange(0, static_cast<int>(incoming.size()));
    refreshTimeDividerRange(0, static_cast<int>(incoming.size()));
    restartTimeDividerRefreshTimer();
    emit countChanged();
}

void ChatMessageModel::updateMessageState(const QString& msgId, const QString& state)
{
    if (msgId.isEmpty())
    {
        return;
    }

    for (int i = 0; i < rowCount(); ++i)
    {
        auto& entry = _items[static_cast<size_t>(i)];
        if (entry.msgId != msgId)
        {
            continue;
        }
        if (entry.messageState == state)
        {
            return;
        }
        entry.messageState = state;
        const QModelIndex idx = index(i, 0);
        emit dataChanged(idx, idx, {MessageStateRole});
        return;
    }
}

bool ChatMessageModel::patchMessageContent(const QString& msgId,
                                           const QString& rawContent,
                                           const QString& state,
                                           qint64 editedAtMs,
                                           qint64 deletedAtMs)
{
    const int row = indexOfMessage(msgId);
    if (row < 0)
    {
        return false;
    }

    auto& entry = _items[static_cast<size_t>(row)];
    bool changed = false;
    QVector<int> roles;

    if (entry.rawContent != rawContent)
    {
        entry.rawContent = rawContent;
        const DecodedMessageContent decoded = MessageContentCodec::decode(rawContent);
        entry.content = decoded.content;
        entry.msgType = decoded.type;
        entry.fileName = decoded.fileName;
        if ((entry.msgType == QStringLiteral("image") || entry.msgType == QStringLiteral("file")) &&
                                             !entry.content.isEmpty())
        {
            entry.content = withDownloadAuth(entry.content);
        }
        entry.isReply = decoded.isReply;
        entry.replyToMsgId = decoded.replyToMsgId;
        entry.replySender = decoded.replySender;
        entry.replyPreview = decoded.replyPreview;
        roles.append(RawContentRole);
        roles.append(ContentRole);
        roles.append(MsgTypeRole);
        roles.append(FileNameRole);
        roles.append(IsReplyRole);
        roles.append(ReplyToMsgIdRole);
        roles.append(ReplySenderRole);
        roles.append(ReplyPreviewRole);
        changed = true;
    }
    if (!state.isEmpty() && entry.messageState != state)
    {
        entry.messageState = state;
        roles.append(MessageStateRole);
        changed = true;
    }
    if (entry.editedAtMs != editedAtMs)
    {
        entry.editedAtMs = editedAtMs;
        roles.append(EditedAtMsRole);
        changed = true;
    }
    if (entry.deletedAtMs != deletedAtMs)
    {
        entry.deletedAtMs = deletedAtMs;
        roles.append(DeletedAtMsRole);
        changed = true;
    }

    if (!changed)
    {
        return false;
    }

    recomputeAvatarFlags();
    refreshSurroundingRows(row, roles);
    return true;
}
