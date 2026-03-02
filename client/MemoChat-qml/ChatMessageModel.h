#ifndef CHATMESSAGEMODEL_H
#define CHATMESSAGEMODEL_H

#include <QAbstractListModel>
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
        SenderNameRole,
        ShowAvatarRole
    };

    explicit ChatMessageModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void clear();
    void setMessages(const std::vector<std::shared_ptr<TextChatData>> &messages, int selfUid);
    void appendMessage(const std::shared_ptr<TextChatData> &message, int selfUid);

signals:
    void countChanged();

private:
    struct MessageEntry {
        QString msgId;
        QString content;
        int fromUid;
        int toUid;
        bool outgoing;
        QString msgType;
        QString fileName;
        QString senderName;
        bool showAvatar;
    };

    MessageEntry toEntry(const std::shared_ptr<TextChatData> &message, int selfUid) const;
    std::vector<MessageEntry> _items;
};

#endif // CHATMESSAGEMODEL_H
