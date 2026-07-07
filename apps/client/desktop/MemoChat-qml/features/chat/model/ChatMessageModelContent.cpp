#include "ChatMessageModel.h"

#include "IconPathUtils.h"
#include "MessageContentCodec.h"

void ChatMessageModel::setDownloadAuthContext(int uid, const QString& token)
{
    Q_UNUSED(uid); // uid is no longer used; auth is token-only via Bearer header
    if (_download_token == token)
    {
        return;
    }
    _download_token = token;
    if (_items.empty())
    {
        return;
    }
    beginResetModel();
    for (auto& entry : _items)
    {
        if ((entry.msgType == QStringLiteral("image") || entry.msgType == QStringLiteral("file")) &&
                                             !entry.content.isEmpty())
        {
            entry.content = withDownloadAuth(entry.content);
        }
        entry.senderIcon = normalizeSenderIcon(entry.senderIcon);
    }
    endResetModel();
}
QString ChatMessageModel::withDownloadAuth(const QString& urlText) const
{
    QUrl url(urlText);
    if (!url.isValid())
    {
        return urlText;
    }
    const QString path = url.path();
    if (!isMediaDownloadPath(path))
    {
        return urlText;
    }
    return resolveMediaDownloadForQml(urlText, _download_token);
}

QString ChatMessageModel::normalizeSenderIcon(const QString& icon) const
{
    const QString trimmed = icon.trimmed();
    if (trimmed.isEmpty())
    {
        return QString();
    }
    return withDownloadAuth(normalizeIconForQml(trimmed));
}

ChatMessageModel::MessageEntry ChatMessageModel::toEntry(const std::shared_ptr<TextChatData>& message,
                                                         int selfUid) const
{
    MessageEntry entry;
    entry.msgId = message->_msg_id;
    entry.rawContent = message->_msg_content;
    const DecodedMessageContent decoded = MessageContentCodec::decode(message->_msg_content);
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
    entry.senderName = message->_from_name;
    entry.senderIcon = normalizeSenderIcon(message->_from_icon);
    entry.fromUid = message->_from_uid;
    entry.fromUserId = message->_from_user_id;
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
    if (entry.messageState.isEmpty())
    {
        if (entry.deletedAtMs > 0)
        {
            entry.messageState = QStringLiteral("deleted");
        }
        else if (entry.editedAtMs > 0)
        {
            entry.messageState = QStringLiteral("edited");
        }
        else
        {
            entry.messageState = QStringLiteral("sent");
        }
    }
    return entry;
}
