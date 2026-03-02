#include "ApplyRequestModel.h"
#include "IconPathUtils.h"

ApplyRequestModel::ApplyRequestModel(QObject *parent)
    : QAbstractListModel(parent),
      _unapproved_count(0)
{
}

int ApplyRequestModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return static_cast<int>(_items.size());
}

QVariant ApplyRequestModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return {};
    }

    const ApplyEntry &entry = _items[static_cast<size_t>(index.row())];
    switch (role) {
    case UidRole:
        return entry.uid;
    case NameRole:
        return entry.name;
    case NickRole:
        return entry.nick;
    case DescRole:
        return entry.desc;
    case IconRole:
        return entry.icon;
    case StatusRole:
        return entry.status;
    case ApprovedRole:
        return entry.status != 0;
    case PendingRole:
        return entry.pending;
    default:
        return {};
    }
}

QHash<int, QByteArray> ApplyRequestModel::roleNames() const
{
    return {
        {UidRole, "uid"},
        {NameRole, "name"},
        {NickRole, "nick"},
        {DescRole, "desc"},
        {IconRole, "icon"},
        {StatusRole, "status"},
        {ApprovedRole, "approved"},
        {PendingRole, "pending"}
    };
}

int ApplyRequestModel::count() const
{
    return rowCount();
}

int ApplyRequestModel::unapprovedCount() const
{
    return _unapproved_count;
}

bool ApplyRequestModel::hasUnapproved() const
{
    return _unapproved_count > 0;
}

void ApplyRequestModel::clear()
{
    if (_items.empty()) {
        return;
    }

    beginResetModel();
    _items.clear();
    endResetModel();
    _unapproved_count = 0;
    emit countChanged();
    emit unapprovedCountChanged();
}

void ApplyRequestModel::setApplies(const std::vector<std::shared_ptr<ApplyInfo>> &applies)
{
    beginResetModel();
    _items.clear();
    _items.reserve(applies.size());

    for (const auto &apply : applies) {
        if (!apply) {
            continue;
        }

        ApplyEntry entry;
        entry.uid = apply->_uid;
        entry.name = apply->_name;
        entry.nick = apply->_nick;
        entry.desc = apply->_desc;
        entry.icon = normalizeIcon(apply->_icon);
        entry.status = apply->_status;
        entry.pending = false;
        _items.push_back(entry);
    }

    endResetModel();
    refreshUnapprovedCount();
    emit countChanged();
}

void ApplyRequestModel::upsertApply(const std::shared_ptr<ApplyInfo> &applyInfo)
{
    if (!applyInfo) {
        return;
    }

    ApplyEntry entry;
    entry.uid = applyInfo->_uid;
    entry.name = applyInfo->_name;
    entry.nick = applyInfo->_nick;
    entry.desc = applyInfo->_desc;
    entry.icon = normalizeIcon(applyInfo->_icon);
    entry.status = applyInfo->_status;
    entry.pending = false;
    upsert(entry);
}

void ApplyRequestModel::upsertApply(const std::shared_ptr<AddFriendApply> &applyInfo)
{
    if (!applyInfo) {
        return;
    }

    ApplyEntry entry;
    entry.uid = applyInfo->_from_uid;
    entry.name = applyInfo->_name;
    entry.nick = applyInfo->_nick;
    entry.desc = applyInfo->_desc;
    entry.icon = normalizeIcon(applyInfo->_icon);
    entry.status = 0;
    entry.pending = false;
    upsert(entry);
}

void ApplyRequestModel::markApproved(int uid)
{
    const int idx = indexOfUid(uid);
    if (idx < 0) {
        return;
    }

    ApplyEntry &entry = _items[static_cast<size_t>(idx)];
    if (entry.status == 1 && !entry.pending) {
        return;
    }

    entry.status = 1;
    entry.pending = false;
    const QModelIndex modelIndex = index(idx, 0);
    emit dataChanged(modelIndex, modelIndex, {StatusRole, ApprovedRole, PendingRole});
    refreshUnapprovedCount();
}

void ApplyRequestModel::setPending(int uid, bool pending)
{
    const int idx = indexOfUid(uid);
    if (idx < 0) {
        return;
    }

    ApplyEntry &entry = _items[static_cast<size_t>(idx)];
    if (entry.pending == pending) {
        return;
    }

    entry.pending = pending;
    const QModelIndex modelIndex = index(idx, 0);
    emit dataChanged(modelIndex, modelIndex, {PendingRole});
    refreshUnapprovedCount();
}

QVariantMap ApplyRequestModel::get(int indexValue) const
{
    if (indexValue < 0 || indexValue >= rowCount()) {
        return {};
    }

    const ApplyEntry &entry = _items[static_cast<size_t>(indexValue)];
    return {
        {"uid", entry.uid},
        {"name", entry.name},
        {"nick", entry.nick},
        {"desc", entry.desc},
        {"icon", entry.icon},
        {"status", entry.status},
        {"approved", entry.status != 0},
        {"pending", entry.pending}
    };
}

int ApplyRequestModel::indexOfUid(int uid) const
{
    for (int i = 0; i < rowCount(); ++i) {
        if (_items[static_cast<size_t>(i)].uid == uid) {
            return i;
        }
    }

    return -1;
}

QString ApplyRequestModel::nameByUid(int uid) const
{
    const int idx = indexOfUid(uid);
    if (idx < 0) {
        return "";
    }

    return _items[static_cast<size_t>(idx)].name;
}

QString ApplyRequestModel::normalizeIcon(QString icon)
{
    return normalizeIconForQml(icon);
}

void ApplyRequestModel::upsert(const ApplyEntry &entry)
{
    const int idx = indexOfUid(entry.uid);
    if (idx >= 0) {
        ApplyEntry &stored = _items[static_cast<size_t>(idx)];
        const bool keepPending = stored.pending && entry.status == 0;
        stored = entry;
        if (keepPending) {
            stored.pending = true;
        }
        const QModelIndex modelIndex = index(idx, 0);
        emit dataChanged(modelIndex, modelIndex);
        refreshUnapprovedCount();
        return;
    }

    beginInsertRows(QModelIndex(), 0, 0);
    _items.insert(_items.begin(), entry);
    endInsertRows();
    refreshUnapprovedCount();
    emit countChanged();
}

void ApplyRequestModel::refreshUnapprovedCount()
{
    int countValue = 0;
    for (const auto &item : _items) {
        if (item.status == 0) {
            ++countValue;
        }
    }

    if (_unapproved_count == countValue) {
        return;
    }

    _unapproved_count = countValue;
    emit unapprovedCountChanged();
}
