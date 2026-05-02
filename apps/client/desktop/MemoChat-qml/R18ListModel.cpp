#include "R18ListModel.h"

R18ListModel::R18ListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int R18ListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return _items.size();
}

QVariant R18ListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= _items.size()) {
        return {};
    }
    const auto& item = _items.at(index.row());
    switch (role) {
    case DataRole: return item;
    case SourceIdRole: return item.value(QStringLiteral("source_id"), item.value(QStringLiteral("id"), item.value(QStringLiteral("key"))));
    case IdRole: return item.value(QStringLiteral("comic_id"), item.value(QStringLiteral("chapter_id"), item.value(QStringLiteral("id"), item.value(QStringLiteral("key")))));
    case TitleRole: return item.value(QStringLiteral("title"), item.value(QStringLiteral("name")));
    case SubtitleRole: return item.value(QStringLiteral("subtitle"), item.value(QStringLiteral("message"), item.value(QStringLiteral("description"))));
    case CoverRole: return item.value(QStringLiteral("cover"));
    case UrlRole: return item.value(QStringLiteral("url"));
    case EnabledRole: return item.value(QStringLiteral("enabled"));
    case StatusRole: return item.value(QStringLiteral("status"), item.value(QStringLiteral("version")));
    case OrderRole: return item.value(QStringLiteral("order"), item.value(QStringLiteral("index")));
    default: return {};
    }
}

QHash<int, QByteArray> R18ListModel::roleNames() const
{
    return {
        {DataRole, "data"},
        {SourceIdRole, "sourceId"},
        {IdRole, "itemId"},
        {TitleRole, "title"},
        {SubtitleRole, "subtitle"},
        {CoverRole, "cover"},
        {UrlRole, "url"},
        {EnabledRole, "enabled"},
        {StatusRole, "status"},
        {OrderRole, "order"},
    };
}

QVariantMap R18ListModel::get(int row) const
{
    if (row < 0 || row >= _items.size()) {
        return {};
    }
    return _items.at(row);
}

void R18ListModel::setItems(const QVariantList& items)
{
    beginResetModel();
    _items.clear();
    _items.reserve(items.size());
    for (const auto& item : items) {
        _items.push_back(item.toMap());
    }
    endResetModel();
    emit countChanged();
}

void R18ListModel::clear()
{
    beginResetModel();
    _items.clear();
    endResetModel();
    emit countChanged();
}
