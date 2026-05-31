#include "MomentsModel.h"

#include <QtGlobal>

void MomentsModel::clear()
{
    if (_items.isEmpty())
    {
        return;
    }
    beginResetModel();
    _items.clear();
    endResetModel();
    emit countChanged();
}

void MomentsModel::setMoments(const QVector<MomentEntry>& moments)
{
    beginResetModel();
    _items = moments;
    endResetModel();
    emit countChanged();
}

void MomentsModel::appendMoments(const QVector<MomentEntry>& moments)
{
    if (moments.isEmpty())
    {
        return;
    }
    int startRow = _items.size();
    beginInsertRows(QModelIndex(), startRow, startRow + moments.size() - 1);
    _items.append(moments);
    endInsertRows();
    emit countChanged();
}

void MomentsModel::upsertMoment(const MomentEntry& moment)
{
    for (int i = 0; i < _items.size(); ++i)
    {
        if (_items[i].momentId == moment.momentId)
        {
            _items[i] = moment;
            QModelIndex idx = index(i);
            emit dataChanged(idx, idx);
            return;
        }
    }
    beginInsertRows(QModelIndex(), _items.size(), _items.size());
    _items.append(moment);
    endInsertRows();
    emit countChanged();
}

void MomentsModel::prependOrUpdateMoment(const MomentEntry& moment)
{
    for (int i = 0; i < _items.size(); ++i)
    {
        if (_items[i].momentId == moment.momentId)
        {
            _items[i] = moment;
            if (i == 0)
            {
                const QModelIndex idx = index(0);
                emit dataChanged(idx, idx);
                return;
            }

            beginMoveRows(QModelIndex(), i, i, QModelIndex(), 0);
            _items.move(i, 0);
            endMoveRows();
            const QModelIndex idx = index(0);
            emit dataChanged(idx, idx);
            return;
        }
    }

    beginInsertRows(QModelIndex(), 0, 0);
    _items.prepend(moment);
    endInsertRows();
    emit countChanged();
}

void MomentsModel::updateLiked(qint64 momentId, bool liked, int likeCount)
{
    for (int i = 0; i < _items.size(); ++i)
    {
        if (_items[i].momentId == momentId)
        {
            _items[i].hasLiked = liked;
            _items[i].likeCount = likeCount;
            QModelIndex idx = index(i);
            emit dataChanged(idx, idx, {HasLikedRole, LikeCountRole});
            return;
        }
    }
}

void MomentsModel::updateCommentCount(qint64 momentId, int delta)
{
    for (int i = 0; i < _items.size(); ++i)
    {
        if (_items[i].momentId == momentId)
        {
            _items[i].commentCount += delta;
            if (_items[i].commentCount < 0)
                _items[i].commentCount = 0;
            QModelIndex idx = index(i);
            emit dataChanged(idx, idx, {CommentCountRole});
            return;
        }
    }
}

void MomentsModel::setCommentCount(qint64 momentId, int count)
{
    for (int i = 0; i < _items.size(); ++i)
    {
        if (_items[i].momentId == momentId)
        {
            _items[i].commentCount = qMax(0, count);
            QModelIndex idx = index(i);
            emit dataChanged(idx, idx, {CommentCountRole});
            return;
        }
    }
}

void MomentsModel::updateDetail(qint64 momentId,
                                const QVector<MomentLike>& likes,
                                const QVector<MomentComment>& comments)
{
    for (int i = 0; i < _items.size(); ++i)
    {
        if (_items[i].momentId == momentId)
        {
            _items[i].likes = likes;
            _items[i].comments = comments;
            QModelIndex idx = index(i);
            emit dataChanged(idx, idx, {LikesRole, CommentsRole});
            return;
        }
    }
}

void MomentsModel::removeMoment(qint64 momentId)
{
    for (int i = 0; i < _items.size(); ++i)
    {
        if (_items[i].momentId == momentId)
        {
            beginRemoveRows(QModelIndex(), i, i);
            _items.remove(i);
            endRemoveRows();
            emit countChanged();
            return;
        }
    }
}
