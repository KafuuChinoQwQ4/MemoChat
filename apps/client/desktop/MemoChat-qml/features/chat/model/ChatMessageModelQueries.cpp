#include "ChatMessageModel.h"

#include "MessageContentCodec.h"

#include <QStringList>

qint64 ChatMessageModel::earliestCreatedAt() const
{
    if (_items.empty())
    {
        return 0;
    }
    return _items.front().createdAt;
}

QString ChatMessageModel::earliestMsgId() const
{
    if (_items.empty())
    {
        return {};
    }
    return _items.front().msgId;
}

bool ChatMessageModel::containsMessage(const QString& msgId) const
{
    return indexOfMessage(msgId) >= 0;
}

QString ChatMessageModel::rawContentByMsgId(const QString& msgId) const
{
    if (msgId.isEmpty())
    {
        return {};
    }
    for (const auto& entry : _items)
    {
        if (entry.msgId == msgId)
        {
            return entry.rawContent;
        }
    }
    return {};
}

QString ChatMessageModel::previewTextByMsgId(const QString& msgId) const
{
    if (msgId.isEmpty())
    {
        return {};
    }
    for (const auto& entry : _items)
    {
        if (entry.msgId == msgId)
        {
            return MessageContentCodec::toPreviewText(entry.rawContent);
        }
    }
    return {};
}

QString ChatMessageModel::exportRecentText(int maxMessages) const
{
    if (_items.empty() || maxMessages <= 0)
    {
        return {};
    }

    const int start = qMax(0, rowCount() - maxMessages);
    QStringList lines;
    for (int i = start; i < rowCount(); ++i)
    {
        const auto& entry = _items[static_cast<size_t>(i)];
        if (entry.deletedAtMs > 0 || entry.msgType != QStringLiteral("text") || entry.content.trimmed().isEmpty())
        {
            continue;
        }
        QString speaker = entry.outgoing ? QStringLiteral("我") : entry.senderName.trimmed();
        if (speaker.isEmpty())
        {
            speaker = QStringLiteral("对方");
        }
        lines.append(QStringLiteral("%1：%2").arg(speaker, entry.content.trimmed()));
    }
    return lines.join(QStringLiteral("\n"));
}

QString ChatMessageModel::latestTextMessage(bool preferIncoming) const
{
    auto findLatest = [this](bool incomingOnly) -> QString
    {
        for (int i = rowCount() - 1; i >= 0; --i)
        {
            const auto& entry = _items[static_cast<size_t>(i)];
            if (entry.deletedAtMs > 0 || entry.msgType != QStringLiteral("text") || entry.content.trimmed().isEmpty())
            {
                continue;
            }
            if (incomingOnly && entry.outgoing)
            {
                continue;
            }
            return entry.content.trimmed();
        }
        return {};
    };

    if (preferIncoming)
    {
        const QString incoming = findLatest(true);
        if (!incoming.isEmpty())
        {
            return incoming;
        }
    }
    return findLatest(false);
}

QVariantMap ChatMessageModel::latestTextMessageInfo(bool preferIncoming) const
{
    auto findLatest = [this](bool incomingOnly) -> QVariantMap
    {
        for (int i = rowCount() - 1; i >= 0; --i)
        {
            const auto& entry = _items[static_cast<size_t>(i)];
            if (entry.deletedAtMs > 0 || entry.msgType != QStringLiteral("text") || entry.content.trimmed().isEmpty())
            {
                continue;
            }
            if (incomingOnly && entry.outgoing)
            {
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

    if (preferIncoming)
    {
        const QVariantMap incoming = findLatest(true);
        if (!incoming.isEmpty())
        {
            return incoming;
        }
    }
    return findLatest(false);
}
