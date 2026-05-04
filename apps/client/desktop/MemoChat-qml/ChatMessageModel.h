#ifndef CHATMESSAGEMODEL_H
#define CHATMESSAGEMODEL_H

#include <QAbstractListModel>
#include <QTimer>
#include <QVariantMap>
#include <QVector>
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
        DeletedAtMsRole
    };

    explicit ChatMessageModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void clear();
    void setMessages(const std::vector<std::shared_ptr<TextChatData>> &messages, int selfUid);
    void setMessagesAtomic(const std::vector<std::shared_ptr<TextChatData>> &messages, int selfUid);
    void appendMessage(const std::shared_ptr<TextChatData> &message, int selfUid);
    void upsertMessage(const std::shared_ptr<TextChatData> &message, int selfUid);
    void updateMessageState(const QString &msgId, const QString &state);
    bool patchMessageContent(const QString &msgId,
                             const QString &rawContent,
                             const QString &state,
                             qint64 editedAtMs,
                             qint64 deletedAtMs);
    void prependMessages(const std::vector<std::shared_ptr<TextChatData>> &messages, int selfUid);
    qint64 earliestCreatedAt() const;
    QString earliestMsgId() const;
    bool containsMessage(const QString &msgId) const;
    QString rawContentByMsgId(const QString &msgId) const;
    QString previewTextByMsgId(const QString &msgId) const;
    Q_INVOKABLE QString exportRecentText(int maxMessages = 80) const;
    Q_INVOKABLE QString latestTextMessage(bool preferIncoming = true) const;
    Q_INVOKABLE QVariantMap latestTextMessageInfo(bool preferIncoming = true) const;
    void setDownloadAuthContext(int uid, const QString &token);

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
    static bool shouldShowAvatarForEntry(const MessageEntry *previous, const MessageEntry &current);
    bool shouldShowTimeDivider(int row) const;
    QString timeDividerText(int row) const;
    void refreshTimeDividerRange(int firstRow, int lastRow);
    void restartTimeDividerRefreshTimer();
    void stopTimeDividerRefreshTimer();
    QString withDownloadAuth(const QString &urlText) const;
    QString normalizeSenderIcon(const QString &icon) const;
    MessageEntry toEntry(const std::shared_ptr<TextChatData> &message, int selfUid) const;
    void recomputeAvatarFlags();
    void refreshAvatarFlags();
    int indexOfMessage(const QString &msgId) const;
    void refreshAvatarRange(int firstRow, int lastRow);
    void refreshSurroundingRows(int centerRow, const QVector<int> &roles);
    int findInsertPosition(const MessageEntry &entry) const;
    std::vector<MessageEntry> _items;
    std::vector<MessageEntry> _itemsBuffer;
    int _download_uid = 0;
    QString _download_token;
    QTimer _time_divider_refresh_timer;
};

#endif // CHATMESSAGEMODEL_H
