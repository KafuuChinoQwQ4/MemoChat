#ifndef CHATMESSAGEMODEL_H
#define CHATMESSAGEMODEL_H

#include <QAbstractListModel>
#include <QtGlobal>
#include <memory>
#include <vector>
#include "userdata.h"

class ChatMessageModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Roles {
        MsgIdRole = Qt::UserRole + 1,
        ContentRole,
        FromUidRole,
        ToUidRole,
        OutgoingRole,
        MsgTypeRole,
        FileNameRole,
        RawContentRole,
        SenderNameRole,
        SenderIconRole,
        ShowAvatarRole,
        CreatedAtRole,
        MessageStateRole,
        IsReplyRole,
        ReplyToMsgIdRole,
        ReplySenderRole,
        ReplyPreviewRole,
        ReplyToServerMsgIdRole,
        ForwardMetaRole,
        EditedAtMsRole,
        DeletedAtMsRole
    };

    explicit ChatMessageModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void clear();
    void setMessages(const std::vector<std::shared_ptr<TextChatData>> &messages, int selfUid);
    void appendMessage(const std::shared_ptr<TextChatData> &message, int selfUid);
    void upsertMessage(const std::shared_ptr<TextChatData> &message, int selfUid);
    void updateMessageState(const QString &msgId, const QString &state);
    void prependMessages(const std::vector<std::shared_ptr<TextChatData>> &messages, int selfUid);
    qint64 earliestCreatedAt() const;
    bool containsMessage(const QString &msgId) const;
    QString rawContentByMsgId(const QString &msgId) const;
    QString previewTextByMsgId(const QString &msgId) const;

signals:
    void countChanged();

private:
    struct MessageEntry {
        QString msgId;
        QString content;
        QString rawContent;
        int fromUid;
        int toUid;
        bool outgoing;
        QString msgType;
        QString fileName;
        QString senderName;
        QString senderIcon;
        bool showAvatar;
        qint64 createdAt;
        qint64 serverMsgId = 0;
        qint64 groupSeq = 0;
        QString messageState;
        bool isReply = false;
        QString replyToMsgId;
        QString replySender;
        QString replyPreview;
        qint64 replyToServerMsgId = 0;
        QString forwardMetaJson;
        qint64 editedAtMs = 0;
        qint64 deletedAtMs = 0;
    };

    static bool lessThan(const MessageEntry &lhs, const MessageEntry &rhs);
    MessageEntry toEntry(const std::shared_ptr<TextChatData> &message, int selfUid) const;
    void refreshAvatarFlags();
    std::vector<MessageEntry> _items;
};

#endif // CHATMESSAGEMODEL_H
