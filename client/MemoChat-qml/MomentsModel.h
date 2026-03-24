#ifndef MOMENTSMODEL_H
#define MOMENTSMODEL_H

#include <QAbstractListModel>
#include <QVariantMap>
#include <QVector>
#include <QString>
#include <QDateTime>
#include <memory>

struct MomentItem {
    int seq = 0;
    QString mediaType;   // text, image, video
    QString mediaKey;
    QString thumbKey;
    QString content;
    int width = 0;
    int height = 0;
    int durationMs = 0;
};

struct MomentEntry {
    qint64 momentId = 0;
    int uid = 0;
    QString userId;
    QString userName;
    QString userNick;
    QString userIcon;
    int visibility = 0;  // 0=public, 1=friends, 2=private
    QString location;
    qint64 createdAt = 0;
    int likeCount = 0;
    int commentCount = 0;
    bool hasLiked = false;
    QVector<MomentItem> items;
};

class MomentsModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Roles {
        MomentIdRole = Qt::UserRole + 1,
        UidRole,
        UserIdRole,
        UserNameRole,
        UserNickRole,
        UserIconRole,
        VisibilityRole,
        LocationRole,
        CreatedAtRole,
        LikeCountRole,
        CommentCountRole,
        HasLikedRole,
        ItemsRole
    };

    explicit MomentsModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const;

    void clear();
    void setMoments(const QVector<MomentEntry>& moments);
    void appendMoments(const QVector<MomentEntry>& moments);
    void upsertMoment(const MomentEntry& moment);
    void updateLiked(qint64 momentId, bool liked, int likeCount);
    void updateCommentCount(qint64 momentId, int delta);
    void removeMoment(qint64 momentId);

    Q_INVOKABLE QVariantMap get(int index) const;
    Q_INVOKABLE MomentEntry getMoment(int index) const;
    Q_INVOKABLE qint64 getMomentId(int index) const;
    Q_INVOKABLE bool getMomentLiked(qint64 momentId) const;

signals:
    void countChanged();

private:
    QVector<MomentEntry> _items;
};

#endif  // MOMENTSMODEL_H
