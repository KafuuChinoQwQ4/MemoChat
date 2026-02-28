#ifndef FRIENDLISTMODEL_H
#define FRIENDLISTMODEL_H

#include <QAbstractListModel>
#include <QVariantMap>
#include <memory>
#include <vector>
#include "userdata.h"

class FriendListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Roles {
        UidRole = Qt::UserRole + 1,
        NameRole,
        NickRole,
        IconRole,
        DescRole,
        LastMsgRole,
        SexRole,
        BackRole
    };

    explicit FriendListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const;

    void clear();
    void setFriends(const std::vector<std::shared_ptr<FriendInfo>> &friends);
    void appendFriends(const std::vector<std::shared_ptr<FriendInfo>> &friends);
    void upsertFriend(const std::shared_ptr<FriendInfo> &friendInfo);
    void upsertFriend(const std::shared_ptr<AuthInfo> &authInfo);
    void upsertFriend(const std::shared_ptr<AuthRsp> &authRsp);
    void updateLastMessage(int uid, const QString &lastMsg);

    Q_INVOKABLE QVariantMap get(int index) const;
    Q_INVOKABLE int indexOfUid(int uid) const;

signals:
    void countChanged();

private:
    struct FriendEntry {
        int uid;
        QString name;
        QString nick;
        QString icon;
        QString desc;
        QString lastMsg;
        int sex;
        QString back;
    };

    static QString normalizeIcon(QString icon);
    void upsert(const FriendEntry &entry);

    std::vector<FriendEntry> _items;
};

#endif // FRIENDLISTMODEL_H
