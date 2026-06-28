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
    enum Roles
    {
        UidRole = Qt::UserRole + 1,
        UserIdRole,
        NameRole,
        NickRole,
        IconRole,
        DescRole,
        LastMsgRole,
        SexRole,
        BackRole,
        DialogTypeRole,
        UnreadCountRole,
        PinnedRankRole,
        DraftTextRole,
        LastMsgTsRole,
        MuteStateRole,
        MentionCountRole,
        SectionKeyRole
    };

    explicit FriendListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const;

    void clear();
    void setFriends(const std::vector<std::shared_ptr<FriendInfo>>& friends);
    void appendFriends(const std::vector<std::shared_ptr<FriendInfo>>& friends);
    void setContactSectionOrderingEnabled(bool enabled);
    void upsertFriend(const std::shared_ptr<FriendInfo>& friendInfo);
    void upsertFriend(const std::shared_ptr<AuthInfo>& authInfo);
    void upsertFriend(const std::shared_ptr<AuthRsp>& authRsp);
    void upsertBatch(const std::vector<std::shared_ptr<FriendInfo>>& friends, bool resetFirst = false);
    void updateLastMessage(int uid, const QString& lastMsg, qint64 lastMsgTs = 0);
    void incrementUnread(int uid, int delta = 1);
    void clearUnread(int uid);
    void incrementMention(int uid, int delta = 1);
    void clearMention(int uid);
    void setMentionCount(int uid, int count);
    void removeByUid(int uid);
    void setDialogMeta(int uid,
                       const QString& dialogType,
                       int unreadCount,
                       int pinnedRank,
                       const QString& draftText,
                       qint64 lastMsgTs,
                       int muteState);

    Q_INVOKABLE QVariantMap get(int index) const;
    Q_INVOKABLE int indexOfUid(int uid) const;

signals:
    void countChanged();

private:
    struct FriendEntry
    {
        int uid = 0;
        QString userId;
        QString name;
        QString nick;
        QString icon;
        QString desc;
        QString lastMsg;
        int sex = 0;
        QString back;
        QString dialogType;
        int unreadCount = 0;
        int pinnedRank = 0;
        QString draftText;
        qint64 lastMsgTs = 0;
        int muteState = 0;
        int mentionCount = 0;
        QString sectionKey;
    };

    static QString normalizeIcon(QString icon);
    static QString displayNameForSection(const FriendEntry& entry);
    static QString sectionKeyForEntry(const FriendEntry& entry);
    static bool contactSectionLessThan(const FriendEntry& lhs, const FriendEntry& rhs);
    static void refreshDerivedFields(FriendEntry& entry);
    static void sortForContactSections(std::vector<FriendEntry>& items);
    FriendEntry toEntry(const std::shared_ptr<FriendInfo>& friendInfo) const;
    FriendEntry mergeWithCurrentEntry(const FriendEntry& entry) const;
    static FriendEntry mergeSparseEntry(const FriendEntry& entry, const FriendEntry& existing);
    void upsert(const FriendEntry& entry);

    std::vector<FriendEntry> _items;
    bool _contact_section_ordering_enabled = false;
};

#endif // FRIENDLISTMODEL_H
