#include "SearchResultModel.h"
#include "IconPathUtils.h"

SearchResultModel::SearchResultModel(QObject *parent)
    : QAbstractListModel(parent),
      _has_result(false),
      _uid(0)
{
}

int SearchResultModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return _has_result ? 1 : 0;
}

QVariant SearchResultModel::data(const QModelIndex &index, int role) const
{
    if (!_has_result || !index.isValid() || index.row() != 0) {
        return {};
    }

    switch (role) {
    case UidRole:
        return _uid;
    case UserIdRole:
        return _user_id;
    case NameRole:
        return _name;
    case NickRole:
        return _nick;
    case DescRole:
        return _desc;
    case IconRole:
        return _icon;
    default:
        return {};
    }
}

QHash<int, QByteArray> SearchResultModel::roleNames() const
{
    return {
        {UidRole, "uid"},
        {UserIdRole, "userId"},
        {NameRole, "name"},
        {NickRole, "nick"},
        {DescRole, "desc"},
        {IconRole, "icon"}
    };
}

void SearchResultModel::clear()
{
    if (!_has_result) {
        return;
    }

    beginResetModel();
    _has_result = false;
    _uid = 0;
    _user_id.clear();
    _name.clear();
    _nick.clear();
    _desc.clear();
    _icon.clear();
    endResetModel();
    emit countChanged();
}

void SearchResultModel::setResult(const std::shared_ptr<SearchInfo> &result)
{
    if (!result) {
        clear();
        return;
    }

    beginResetModel();
    _has_result = true;
    _uid = result->_uid;
    _user_id = result->_user_id;
    _name = result->_name;
    _nick = result->_nick;
    _desc = result->_desc;
    _icon = normalizeIconForQml(result->_icon);
    endResetModel();
    emit countChanged();
}
