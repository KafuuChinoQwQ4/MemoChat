#ifndef SEARCHRESULTMODEL_H
#define SEARCHRESULTMODEL_H

#include <QAbstractListModel>
#include <memory>
#include "userdata.h"

class SearchResultModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Roles {
        UidRole = Qt::UserRole + 1,
        NameRole,
        NickRole,
        DescRole,
        IconRole
    };

    explicit SearchResultModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void clear();
    void setResult(const std::shared_ptr<SearchInfo> &result);

signals:
    void countChanged();

private:
    bool _has_result;
    int _uid;
    QString _name;
    QString _nick;
    QString _desc;
    QString _icon;
};

#endif // SEARCHRESULTMODEL_H

