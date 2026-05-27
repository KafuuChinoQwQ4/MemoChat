#include "AgentMessageModel.h"
#include <algorithm>
#include <QRegularExpression>

namespace {
struct AssistantParts {
    QString thinking;
    QString answer;
};

AssistantParts splitAssistantParts(const QString& raw)
{
    AssistantParts parts;
    if (raw.isEmpty()) {
        return parts;
    }

    QString answer = raw;
    QRegularExpression closedBlock(QStringLiteral("<think>([\\s\\S]*?)</think>"),
                                   QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator it = closedBlock.globalMatch(raw);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        const QString block = match.captured(1).trimmed();
        if (!block.isEmpty()) {
            if (!parts.thinking.isEmpty()) {
                parts.thinking += QStringLiteral("\n\n");
            }
            parts.thinking += block;
        }
    }
    answer.remove(closedBlock);

    QRegularExpression openBlock(QStringLiteral("<think>([\\s\\S]*)$"),
                                 QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch openMatch = openBlock.match(answer);
    if (openMatch.hasMatch()) {
        const QString block = openMatch.captured(1).trimmed();
        if (!block.isEmpty()) {
            if (!parts.thinking.isEmpty()) {
                parts.thinking += QStringLiteral("\n\n");
            }
            parts.thinking += block;
        }
        answer = answer.left(openMatch.capturedStart()).trimmed();
    }

    parts.answer = answer.trimmed();
    return parts;
}
}

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
    case ThinkingContentRole: return entry.thinkingContent;
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
        {ThinkingContentRole, "thinkingContent"},
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
    const int row = indexOfMessage(msgId);
    if (row < 0) return;

    MessageEntry* entry = &_items[row];
    const AssistantParts parts = splitAssistantParts(chunk);
    entry->thinkingContent = parts.thinking;
    entry->streamingContent = parts.answer;
    entry->content = entry->streamingContent;

    QModelIndex idx = index(row, 0);
    emit dataChanged(idx, idx, {ContentRole, StreamingContentRole, ThinkingContentRole, IsStreamingRole});
}

void AgentMessageModel::finalizeAIMessage(const QString& msgId)
{
    const int row = indexOfMessage(msgId);
    if (row < 0) return;

    MessageEntry* entry = &_items[row];
    entry->content = entry->streamingContent;
    entry->isStreaming = false;

    QModelIndex idx = index(row, 0);
    emit dataChanged(idx, idx, {ContentRole, IsStreamingRole, StreamingContentRole, ThinkingContentRole});
}

void AgentMessageModel::setError(const QString& msgId, const QString& error)
{
    const int row = indexOfMessage(msgId);
    if (row < 0) return;

    MessageEntry* entry = &_items[row];
    entry->errorMessage = error;
    entry->isStreaming = false;

    QModelIndex idx = index(row, 0);
    emit dataChanged(idx, idx, {ErrorMessageRole, IsStreamingRole});
}

void AgentMessageModel::setSources(const QString& msgId, const QString& sourcesJson)
{
    const int row = indexOfMessage(msgId);
    if (row < 0) return;

    MessageEntry* entry = &_items[row];
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
