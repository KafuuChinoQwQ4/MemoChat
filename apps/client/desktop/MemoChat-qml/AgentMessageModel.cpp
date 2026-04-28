#include "AgentMessageModel.h"
#include <algorithm>

AgentMessageModel::AgentMessageModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int AgentMessageModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return _items.size();
}

QVariant AgentMessageModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return {};
    }

    const MessageEntry& entry = _items.at(index.row());
    switch (role) {
    case MsgIdRole:       return entry.msgId;
    case ContentRole:      return entry.isStreaming ? entry.streamingContent : entry.content;
    case RoleRole:        return entry.role;
    case CreatedAtRole:    return entry.createdAt;
    case IsUserRole:       return entry.role == "user";
    case IsAssistantRole:  return entry.role == "assistant";
    case IsStreamingRole:  return entry.isStreaming;
    case StreamingContentRole: return entry.streamingContent;
    case TokensUsedRole:   return entry.tokensUsed;
    case ModelNameRole:    return entry.modelName;
    case ErrorMessageRole: return entry.errorMessage;
    case SourcesRole:      return entry.sourcesJson;
    default:              return {};
    }
}

QHash<int, QByteArray> AgentMessageModel::roleNames() const
{
    return {
        {MsgIdRole,        "msgId"},
        {ContentRole,      "content"},
        {RoleRole,         "role"},
        {CreatedAtRole,    "createdAt"},
        {IsUserRole,       "isUser"},
        {IsAssistantRole,  "isAssistant"},
        {IsStreamingRole,   "isStreaming"},
        {StreamingContentRole, "streamingContent"},
        {TokensUsedRole,   "tokensUsed"},
        {ModelNameRole,     "modelName"},
        {ErrorMessageRole,  "errorMessage"},
        {SourcesRole,       "sources"},
    };
}

void AgentMessageModel::clear()
{
    if (_items.isEmpty()) return;
    beginResetModel();
    _items.clear();
    endResetModel();
    emit countChanged();
}

void AgentMessageModel::appendUserMessage(const QString& content)
{
    MessageEntry entry;
    entry.msgId = QString("user_") + QString::number(QDateTime::currentMSecsSinceEpoch());
    entry.content = content;
    entry.streamingContent = content;
    entry.role = "user";
    entry.createdAt = QDateTime::currentMSecsSinceEpoch();

    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    _items.append(entry);
    endInsertRows();
    emit countChanged();
}

void AgentMessageModel::appendAIMessage(const QString& msgId, const QString& model)
{
    MessageEntry entry;
    entry.msgId = msgId.isEmpty()
        ? QString("ai_") + QString::number(QDateTime::currentMSecsSinceEpoch())
        : msgId;
    entry.content = "";
    entry.streamingContent = "";
    entry.role = "assistant";
    entry.modelName = model;
    entry.createdAt = QDateTime::currentMSecsSinceEpoch();
    entry.isStreaming = true;

    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    _items.append(entry);
    endInsertRows();
    emit countChanged();
}

void AgentMessageModel::updateStreamingContent(const QString& msgId, const QString& chunk)
{
    MessageEntry* entry = findEntry(msgId);
    if (!entry) return;

    int row = _items.indexOf(*entry);
    if (row < 0) return;

    entry->streamingContent += chunk;
    entry->content = entry->streamingContent;

    QModelIndex idx = index(row, 0);
    emit dataChanged(idx, idx, {ContentRole, StreamingContentRole, IsStreamingRole});
}

void AgentMessageModel::finalizeAIMessage(const QString& msgId)
{
    MessageEntry* entry = findEntry(msgId);
    if (!entry) return;

    int row = _items.indexOf(*entry);
    if (row < 0) return;

    entry->content = entry->streamingContent;
    entry->isStreaming = false;

    QModelIndex idx = index(row, 0);
    emit dataChanged(idx, idx, {ContentRole, IsStreamingRole, StreamingContentRole});
}

void AgentMessageModel::setError(const QString& msgId, const QString& error)
{
    MessageEntry* entry = findEntry(msgId);
    if (!entry) return;

    int row = _items.indexOf(*entry);
    if (row < 0) return;

    entry->errorMessage = error;
    entry->isStreaming = false;

    QModelIndex idx = index(row, 0);
    emit dataChanged(idx, idx, {ErrorMessageRole, IsStreamingRole});
}

void AgentMessageModel::setSources(const QString& msgId, const QString& sourcesJson)
{
    MessageEntry* entry = findEntry(msgId);
    if (!entry) return;

    int row = _items.indexOf(*entry);
    if (row < 0) return;

    entry->sourcesJson = sourcesJson;

    QModelIndex idx = index(row, 0);
    emit dataChanged(idx, idx, {SourcesRole});
}

int AgentMessageModel::indexOfMessage(const QString& msgId) const
{
    for (int i = 0; i < _items.size(); ++i) {
        if (_items.at(i).msgId == msgId) return i;
    }
    return -1;
}

AgentMessageModel::MessageEntry* AgentMessageModel::findEntry(const QString& msgId)
{
    int idx = indexOfMessage(msgId);
    if (idx < 0) return nullptr;
    return &_items[idx];
}
