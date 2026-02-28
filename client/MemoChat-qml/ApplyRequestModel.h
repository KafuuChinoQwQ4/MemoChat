#ifndef APPLYREQUESTMODEL_H
#define APPLYREQUESTMODEL_H

#include <QAbstractListModel>
#include <QVariantMap>
#include <memory>
#include <vector>
#include "userdata.h"

class ApplyRequestModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(int unapprovedCount READ unapprovedCount NOTIFY unapprovedCountChanged)
    Q_PROPERTY(bool hasUnapproved READ hasUnapproved NOTIFY unapprovedCountChanged)

public:
    enum Roles {
        UidRole = Qt::UserRole + 1,
        NameRole,
        NickRole,
        DescRole,
        IconRole,
        StatusRole,
        ApprovedRole,
        PendingRole
    };

    explicit ApplyRequestModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const;
    int unapprovedCount() const;
    bool hasUnapproved() const;

    void clear();
    void setApplies(const std::vector<std::shared_ptr<ApplyInfo>> &applies);
    void upsertApply(const std::shared_ptr<ApplyInfo> &applyInfo);
    void upsertApply(const std::shared_ptr<AddFriendApply> &applyInfo);
    void markApproved(int uid);
    void setPending(int uid, bool pending);

    Q_INVOKABLE QVariantMap get(int index) const;
    Q_INVOKABLE int indexOfUid(int uid) const;
    Q_INVOKABLE QString nameByUid(int uid) const;

signals:
    void countChanged();
    void unapprovedCountChanged();

private:
    struct ApplyEntry {
        int uid;
        QString name;
        QString nick;
        QString desc;
        QString icon;
        int status;
        bool pending;
    };

    static QString normalizeIcon(QString icon);
    void upsert(const ApplyEntry &entry);
    void refreshUnapprovedCount();

    std::vector<ApplyEntry> _items;
    int _unapproved_count;
};

#endif // APPLYREQUESTMODEL_H
