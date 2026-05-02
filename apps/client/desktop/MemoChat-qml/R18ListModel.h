#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QVariantMap>
#include <QVector>

class R18ListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Roles {
        DataRole = Qt::UserRole + 1,
        SourceIdRole,
        IdRole,
        TitleRole,
        SubtitleRole,
        CoverRole,
        UrlRole,
        EnabledRole,
        StatusRole,
        OrderRole
    };

    explicit R18ListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int count() const { return _items.size(); }
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE QVariantMap get(int row) const;

    void setItems(const QVariantList& items);
    void clear();

signals:
    void countChanged();

private:
    QVector<QVariantMap> _items;
};
