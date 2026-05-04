#include "ChatMessageModel.h"
#include "IconPathUtils.h"
#include "MessageContentCodec.h"
#include <algorithm>
#include <limits>
#include <unordered_set>
#include <QDateTime>
#include <QStringList>
#include <QUrlQuery>

namespace {

QString weekdayText(int dayOfWeek)
{
    switch (dayOfWeek) {
    case 1:
        return QStringLiteral("周一");
    case 2:
        return QStringLiteral("周二");
    case 3:
        return QStringLiteral("周三");
    case 4:
        return QStringLiteral("周四");
    case 5:
        return QStringLiteral("周五");
    case 6:
        return QStringLiteral("周六");
    case 7:
        return QStringLiteral("周日");
    default:
        return QString();
    }
}

QDateTime localDateTimeForMs(qint64 createdAt)
{
    return QDateTime::fromMSecsSinceEpoch(createdAt, Qt::LocalTime);
}

bool sameMinuteBucket(const QDateTime &lhs, const QDateTime &rhs)
{
    return lhs.date() == rhs.date()
           && lhs.time().hour() == rhs.time().hour()
           && lhs.time().minute() == rhs.time().minute();
}

constexpr qint64 kAvatarGroupWindowMs = 60000;

}

ChatMessageModel::ChatMessageModel(QObject *parent)
    : QAbstractListModel(parent)
{
    _time_divider_refresh_timer.setSingleShot(true);
    connect(&_time_divider_refresh_timer, &QTimer::timeout, this, [this]() {
        if (_items.empty()) {
            return;
        }
        refreshTimeDividerRange(0, rowCount() - 1);
        restartTimeDividerRefreshTimer();
    });
}

bool ChatMessageModel::lessThan(const MessageEntry &lhs, const MessageEntry &rhs)
{
    if (lhs.groupSeq > 0 && rhs.groupSeq > 0 && lhs.groupSeq != rhs.groupSeq) {
        return lhs.groupSeq < rhs.groupSeq;
    }
    if (lhs.createdAt != rhs.createdAt) {
        return lhs.createdAt < rhs.createdAt;
    }
    if (lhs.serverMsgId > 0 && rhs.serverMsgId > 0 && lhs.serverMsgId != rhs.serverMsgId) {
        return lhs.serverMsgId < rhs.serverMsgId;
    }
    return lhs.msgId < rhs.msgId;
}

bool ChatMessageModel::shouldShowAvatarForEntry(const MessageEntry *previous, const MessageEntry &current)
{
    if (!previous) {
        return true;
    }
    if (current.fromUid != previous->fromUid) {
        return true;
    }
    if (current.createdAt <= 0 || previous->createdAt <= 0) {
        return true;
    }
    return (current.createdAt - previous->createdAt) > kAvatarGroupWindowMs;
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
    return {
        {MsgIdRole, "msgId"},
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
        {DeletedAtMsRole, "deletedAtMs"}
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
    stopTimeDividerRefreshTimer();
    emit countChanged();
}

void ChatMessageModel::setMessages(const std::vector<std::shared_ptr<TextChatData> > &messages, int selfUid)
{
    // 使用双缓冲：先在缓冲区构建，然后原子切换
    setMessagesAtomic(messages, selfUid);
}

void ChatMessageModel::setMessagesAtomic(const std::vector<std::shared_ptr<TextChatData> > &messages, int selfUid)
{
    const qint64 startTs = QDateTime::currentMSecsSinceEpoch();

    // 先在缓冲区构建数据
    _itemsBuffer.clear();
    _itemsBuffer.reserve(messages.size());

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

void ChatMessageModel::appendMessage(const std::shared_ptr<TextChatData> &message, int selfUid)
{
    if (!message) {
        return;
    }

    MessageEntry entry = toEntry(message, selfUid);
    if (indexOfMessage(entry.msgId) >= 0) {
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

void ChatMessageModel::upsertMessage(const std::shared_ptr<TextChatData> &message, int selfUid)
{
    if (!message) {
        return;
    }

    MessageEntry entry = toEntry(message, selfUid);
    const int existingIndex = indexOfMessage(entry.msgId);
    if (existingIndex < 0) {
        appendMessage(message, selfUid);
        return;
    }

    const MessageEntry existing = _items[static_cast<size_t>(existingIndex)];
    const bool orderUnchanged = (existing.groupSeq == entry.groupSeq)
        && (existing.createdAt == entry.createdAt)
        && (existing.serverMsgId == entry.serverMsgId);
    if (orderUnchanged) {
        _items[static_cast<size_t>(existingIndex)] = entry;
        recomputeAvatarFlags();
        refreshSurroundingRows(existingIndex,
                               {ContentRole,
                                RawContentRole,
                                OutgoingRole,
                                MsgTypeRole,
                                FileNameRole,
                                SenderNameRole,
                                SenderIconRole,
                                ShowAvatarRole,
                                CreatedAtRole,
                                ShowTimeDividerRole,
                                TimeDividerTextRole,
                                MessageStateRole,
                                IsReplyRole,
                                ReplyToMsgIdRole,
                                ReplySenderRole,
                                ReplyPreviewRole,
                                ReplyToServerMsgIdRole,
                                ForwardMetaRole,
                                EditedAtMsRole,
                                DeletedAtMsRole});
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
        if (indexOfMessage(entry.msgId) >= 0) {
            continue;
        }
        incoming.push_back(std::move(entry));
    }
    if (incoming.empty()) {
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

void ChatMessageModel::updateMessageState(const QString &msgId, const QString &state)
{
    if (msgId.isEmpty()) {
        return;
    }

    for (int i = 0; i < rowCount(); ++i) {
        auto &entry = _items[static_cast<size_t>(i)];
        if (entry.msgId != msgId) {
            continue;
        }
        if (entry.messageState == state) {
            return;
        }
        entry.messageState = state;
        const QModelIndex idx = index(i, 0);
        emit dataChanged(idx, idx, {MessageStateRole});
        return;
    }
}

bool ChatMessageModel::patchMessageContent(const QString &msgId,
                                           const QString &rawContent,
                                           const QString &state,
                                           qint64 editedAtMs,
                                           qint64 deletedAtMs)
{
    const int row = indexOfMessage(msgId);
    if (row < 0) {
        return false;
    }

    auto &entry = _items[static_cast<size_t>(row)];
    bool changed = false;
    QVector<int> roles;

    if (entry.rawContent != rawContent) {
        entry.rawContent = rawContent;
        const DecodedMessageContent decoded = MessageContentCodec::decode(rawContent);
        entry.content = decoded.content;
        entry.msgType = decoded.type;
        entry.fileName = decoded.fileName;
        if ((entry.msgType == QStringLiteral("image") || entry.msgType == QStringLiteral("file"))
            && !entry.content.isEmpty()) {
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
    if (!state.isEmpty() && entry.messageState != state) {
        entry.messageState = state;
        roles.append(MessageStateRole);
        changed = true;
    }
    if (entry.editedAtMs != editedAtMs) {
        entry.editedAtMs = editedAtMs;
        roles.append(EditedAtMsRole);
        changed = true;
    }
    if (entry.deletedAtMs != deletedAtMs) {
        entry.deletedAtMs = deletedAtMs;
        roles.append(DeletedAtMsRole);
        changed = true;
    }

    if (!changed) {
        return false;
    }

    recomputeAvatarFlags();
    refreshSurroundingRows(row, roles);
    return true;
}

qint64 ChatMessageModel::earliestCreatedAt() const
{
    if (_items.empty()) {
        return 0;
    }
    return _items.front().createdAt;
}

QString ChatMessageModel::earliestMsgId() const
{
    if (_items.empty()) {
        return {};
    }
    return _items.front().msgId;
}

bool ChatMessageModel::containsMessage(const QString &msgId) const
{
    return indexOfMessage(msgId) >= 0;
}

QString ChatMessageModel::rawContentByMsgId(const QString &msgId) const
{
    if (msgId.isEmpty()) {
        return {};
    }
    for (const auto &entry : _items) {
        if (entry.msgId == msgId) {
            return entry.rawContent;
        }
    }
    return {};
}

QString ChatMessageModel::previewTextByMsgId(const QString &msgId) const
{
    if (msgId.isEmpty()) {
        return {};
    }
    for (const auto &entry : _items) {
        if (entry.msgId == msgId) {
            return MessageContentCodec::toPreviewText(entry.rawContent);
        }
    }
    return {};
}

QString ChatMessageModel::exportRecentText(int maxMessages) const
{
    if (_items.empty() || maxMessages <= 0) {
        return {};
    }

    const int start = qMax(0, rowCount() - maxMessages);
    QStringList lines;
    for (int i = start; i < rowCount(); ++i) {
        const auto &entry = _items[static_cast<size_t>(i)];
        if (entry.deletedAtMs > 0 || entry.msgType != QStringLiteral("text") || entry.content.trimmed().isEmpty()) {
            continue;
        }
        QString speaker = entry.outgoing ? QStringLiteral("我") : entry.senderName.trimmed();
        if (speaker.isEmpty()) {
            speaker = QStringLiteral("对方");
        }
        lines.append(QStringLiteral("%1：%2").arg(speaker, entry.content.trimmed()));
    }
    return lines.join(QStringLiteral("\n"));
}

QString ChatMessageModel::latestTextMessage(bool preferIncoming) const
{
    auto findLatest = [this](bool incomingOnly) -> QString {
        for (int i = rowCount() - 1; i >= 0; --i) {
            const auto &entry = _items[static_cast<size_t>(i)];
            if (entry.deletedAtMs > 0 || entry.msgType != QStringLiteral("text") || entry.content.trimmed().isEmpty()) {
                continue;
            }
            if (incomingOnly && entry.outgoing) {
                continue;
            }
            return entry.content.trimmed();
        }
        return {};
    };

    if (preferIncoming) {
        const QString incoming = findLatest(true);
        if (!incoming.isEmpty()) {
            return incoming;
        }
    }
    return findLatest(false);
}

QVariantMap ChatMessageModel::latestTextMessageInfo(bool preferIncoming) const
{
    auto findLatest = [this](bool incomingOnly) -> QVariantMap {
        for (int i = rowCount() - 1; i >= 0; --i) {
            const auto &entry = _items[static_cast<size_t>(i)];
            if (entry.deletedAtMs > 0 || entry.msgType != QStringLiteral("text") || entry.content.trimmed().isEmpty()) {
                continue;
            }
            if (incomingOnly && entry.outgoing) {
                continue;
            }
            return QVariantMap{
                {QStringLiteral("msgId"), entry.msgId},
                {QStringLiteral("content"), entry.content.trimmed()},
                {QStringLiteral("senderName"), entry.senderName},
                {QStringLiteral("outgoing"), entry.outgoing},
            };
        }
        return {};
    };

    if (preferIncoming) {
        const QVariantMap incoming = findLatest(true);
        if (!incoming.isEmpty()) {
            return incoming;
        }
    }
    return findLatest(false);
}

void ChatMessageModel::setDownloadAuthContext(int uid, const QString &token)
{
    if (_download_uid == uid && _download_token == token) {
        return;
    }
    _download_uid = uid;
    _download_token = token;
    if (_items.empty()) {
        return;
    }
    beginResetModel();
    for (auto &entry : _items) {
        if ((entry.msgType == QStringLiteral("image") || entry.msgType == QStringLiteral("file"))
            && !entry.content.isEmpty()) {
            entry.content = withDownloadAuth(entry.content);
        }
        entry.senderIcon = normalizeSenderIcon(entry.senderIcon);
    }
    endResetModel();
}

bool ChatMessageModel::shouldShowTimeDivider(int row) const
{
    if (row < 0 || row >= rowCount()) {
        return false;
    }
    if (row == 0) {
        return true;
    }

    const QDateTime current = localDateTimeForMs(_items[static_cast<size_t>(row)].createdAt);
    const QDateTime previous = localDateTimeForMs(_items[static_cast<size_t>(row - 1)].createdAt);
    if (current.date() != previous.date()) {
        return true;
    }
    return current.date() == QDate::currentDate() && !sameMinuteBucket(previous, current);
}

QString ChatMessageModel::timeDividerText(int row) const
{
    if (!shouldShowTimeDivider(row)) {
        return QString();
    }

    const QDateTime messageDateTime = localDateTimeForMs(_items[static_cast<size_t>(row)].createdAt);
    const QDate currentDate = QDate::currentDate();
    const QDate messageDate = messageDateTime.date();
    if (messageDate == currentDate) {
        return messageDateTime.toString(QStringLiteral("HH:mm"));
    }

    const int daysAgo = messageDate.daysTo(currentDate);
    if (daysAgo >= 1 && daysAgo <= 7) {
        return QStringLiteral("%1 %2")
            .arg(weekdayText(messageDate.dayOfWeek()), messageDateTime.toString(QStringLiteral("HH:mm")));
    }
    if (messageDate.year() == currentDate.year()) {
        return messageDateTime.toString(QStringLiteral("M月d日 HH:mm"));
    }
    return messageDateTime.toString(QStringLiteral("yyyy年M月d日 HH:mm"));
}

void ChatMessageModel::refreshTimeDividerRange(int firstRow, int lastRow)
{
    if (_items.empty()) {
        return;
    }

    const int topRow = qMax(0, firstRow);
    const int bottomRow = qMin(rowCount() - 1, lastRow);
    if (topRow > bottomRow) {
        return;
    }

    const QModelIndex top = index(topRow, 0);
    const QModelIndex bottom = index(bottomRow, 0);
    emit dataChanged(top, bottom, {ShowTimeDividerRole, TimeDividerTextRole});
}

void ChatMessageModel::restartTimeDividerRefreshTimer()
{
    if (_items.empty()) {
        _time_divider_refresh_timer.stop();
        return;
    }

    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    const int delayMs = static_cast<int>(60000 - (nowMs % 60000));
    _time_divider_refresh_timer.start(qMax(1, delayMs));
}

void ChatMessageModel::stopTimeDividerRefreshTimer()
{
    _time_divider_refresh_timer.stop();
}

QString ChatMessageModel::withDownloadAuth(const QString &urlText) const
{
    if (_download_uid <= 0 || _download_token.trimmed().isEmpty()) {
        return urlText;
    }
    QUrl url(urlText);
    if (!url.isValid()) {
        return urlText;
    }
    const QString path = url.path();
    if (!path.endsWith("/media/download") && !path.contains("/media/download")) {
        return urlText;
    }
    QUrlQuery query(url);
    query.removeQueryItem("uid");
    query.removeQueryItem("token");
    query.addQueryItem("uid", QString::number(_download_uid));
    query.addQueryItem("token", _download_token);
    url.setQuery(query);
    return url.toString();
}

QString ChatMessageModel::normalizeSenderIcon(const QString &icon) const
{
    const QString trimmed = icon.trimmed();
    if (trimmed.isEmpty()) {
        return QString();
    }
    return withDownloadAuth(normalizeIconForQml(trimmed));
}

ChatMessageModel::MessageEntry ChatMessageModel::toEntry(const std::shared_ptr<TextChatData> &message, int selfUid) const
{
    MessageEntry entry;
    entry.msgId = message->_msg_id;
    entry.rawContent = message->_msg_content;
    const DecodedMessageContent decoded = MessageContentCodec::decode(message->_msg_content);
    entry.content = decoded.content;
    entry.msgType = decoded.type;
    entry.fileName = decoded.fileName;
    if ((entry.msgType == QStringLiteral("image") || entry.msgType == QStringLiteral("file"))
        && !entry.content.isEmpty()) {
        entry.content = withDownloadAuth(entry.content);
    }
    entry.isReply = decoded.isReply;
    entry.replyToMsgId = decoded.replyToMsgId;
    entry.replySender = decoded.replySender;
    entry.replyPreview = decoded.replyPreview;
    entry.senderName = message->_from_name;
    entry.senderIcon = normalizeSenderIcon(message->_from_icon);
    entry.fromUid = message->_from_uid;
    entry.toUid = message->_to_uid;
    entry.outgoing = (message->_from_uid == selfUid);
    entry.showAvatar = true;
    entry.createdAt = message->_created_at;
    entry.serverMsgId = message->_server_msg_id;
    entry.groupSeq = message->_group_seq;
    entry.messageState = message->_msg_state;
    entry.replyToServerMsgId = message->_reply_to_server_msg_id;
    entry.forwardMetaJson = message->_forward_meta_json;
    entry.editedAtMs = message->_edited_at_ms;
    entry.deletedAtMs = message->_deleted_at_ms;
    if (entry.messageState.isEmpty()) {
        if (entry.deletedAtMs > 0) {
            entry.messageState = QStringLiteral("deleted");
        } else if (entry.editedAtMs > 0) {
            entry.messageState = QStringLiteral("edited");
        } else {
            entry.messageState = QStringLiteral("sent");
        }
    }
    return entry;
}

void ChatMessageModel::refreshAvatarFlags()
{
    recomputeAvatarFlags();
    if (!_items.empty()) {
        const QModelIndex top = index(0, 0);
        const QModelIndex bottom = index(rowCount() - 1, 0);
        emit dataChanged(top, bottom, {ShowAvatarRole});
    }
}

void ChatMessageModel::recomputeAvatarFlags()
{
    const MessageEntry *previous = nullptr;
    for (auto &entry : _items) {
        entry.showAvatar = shouldShowAvatarForEntry(previous, entry);
        previous = &entry;
    }
}

int ChatMessageModel::indexOfMessage(const QString &msgId) const
{
    if (msgId.isEmpty()) {
        return -1;
    }
    for (int i = 0; i < rowCount(); ++i) {
        if (_items[static_cast<size_t>(i)].msgId == msgId) {
            return i;
        }
    }
    return -1;
}

void ChatMessageModel::refreshAvatarRange(int firstRow, int lastRow)
{
    if (_items.empty()) {
        return;
    }

    const int topRow = qMax(0, firstRow);
    const int bottomRow = qMin(rowCount() - 1, lastRow);
    if (topRow > bottomRow) {
        return;
    }

    emit dataChanged(index(topRow, 0), index(bottomRow, 0), {ShowAvatarRole});
}

void ChatMessageModel::refreshSurroundingRows(int centerRow, const QVector<int> &roles)
{
    if (_items.empty() || centerRow < 0 || centerRow >= rowCount()) {
        return;
    }

    const int topRow = qMax(0, centerRow - 1);
    const int bottomRow = qMin(rowCount() - 1, centerRow + 1);
    QVector<int> mergedRoles = roles;
    mergedRoles.append(ShowAvatarRole);
    mergedRoles.append(ShowTimeDividerRole);
    mergedRoles.append(TimeDividerTextRole);
    emit dataChanged(index(topRow, 0), index(bottomRow, 0), mergedRoles);
}

int ChatMessageModel::findInsertPosition(const MessageEntry &entry) const
{
    if (_items.empty()) {
        return 0;
    }

    const auto insertIt = std::upper_bound(
        _items.begin(), _items.end(), entry,
        [](const MessageEntry &lhs, const MessageEntry &rhs) {
            return ChatMessageModel::lessThan(lhs, rhs);
        });
    return static_cast<int>(std::distance(_items.begin(), insertIt));
}
