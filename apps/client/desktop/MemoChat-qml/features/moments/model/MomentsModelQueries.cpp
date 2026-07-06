#include "MomentsModel.h"

QVariantMap MomentsModel::get(int index) const
{
    QVariantMap map;
    if (index < 0 || index >= _items.size())
    {
        return map;
    }
    const auto& item = _items.at(index);
    const QModelIndex modelIndex = createIndex(index, 0);
    map["momentId"] = item.momentId;
    map["uid"] = item.uid;
    map["userId"] = item.userId;
    map["userName"] = item.userName;
    map["userNick"] = item.userNick;
    map["userIcon"] = item.userIcon;
    map["visibility"] = item.visibility;
    map["location"] = item.location;
    map["createdAt"] = item.createdAt;
    map["likeCount"] = item.likeCount;
    map["commentCount"] = item.commentCount;
    map["hasLiked"] = item.hasLiked;
    map["items"] = data(modelIndex, ItemsRole);
    map["textContent"] = data(modelIndex, TextContentRole);
    map["likes"] = data(modelIndex, LikesRole);
    map["comments"] = data(modelIndex, CommentsRole);
    return map;
}

MomentEntry MomentsModel::getMoment(int index) const
{
    if (index < 0 || index >= _items.size())
    {
        return MomentEntry();
    }
    return _items.at(index);
}

MomentEntry MomentsModel::getMomentById(qint64 momentId) const
{
    for (const auto& item : _items)
    {
        if (item.momentId == momentId)
        {
            return item;
        }
    }
    return MomentEntry();
}

qint64 MomentsModel::getMomentId(int index) const
{
    if (index < 0 || index >= _items.size())
    {
        return 0;
    }
    return _items.at(index).momentId;
}

bool MomentsModel::getMomentLiked(qint64 momentId) const
{
    for (const auto& item : _items)
    {
        if (item.momentId == momentId)
        {
            return item.hasLiked;
        }
    }
    return false;
}

int MomentsModel::getMomentLikeCount(qint64 momentId) const
{
    for (const auto& item : _items)
    {
        if (item.momentId == momentId)
        {
            return item.likeCount;
        }
    }
    return 0;
}

bool MomentsModel::hasMoment(qint64 momentId) const
{
    for (const auto& item : _items)
    {
        if (item.momentId == momentId)
        {
            return true;
        }
    }
    return false;
}

QVariantMap MomentsModel::snapshotMoment(qint64 momentId) const
{
    for (int index = 0; index < _items.size(); ++index)
    {
        if (_items.at(index).momentId != momentId)
        {
            continue;
        }
        return get(index);
    }
    return QVariantMap();
}
