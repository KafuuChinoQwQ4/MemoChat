#include "ChatMessageModel.h"

#include <QDateTime>
#include <QTimeZone>
#include <algorithm>

namespace
{
QString weekdayText(int dayOfWeek)
{
    switch (dayOfWeek)
    {
        case 1:
            return QStringLiteral("周一");
        case 2:
            return QStringLiteral("周二");
        case 3:
            return QStringLiteral("周三");
        case 4:
            return QStringLiteral("周四");
        case 5:
            return QStringLiteral("周五");
        case 6:
            return QStringLiteral("周六");
        case 7:
            return QStringLiteral("周日");
        default:
            return QString();
    }
}

QDateTime localDateTimeForMs(qint64 createdAt)
{
    return QDateTime::fromMSecsSinceEpoch(createdAt, QTimeZone(QTimeZone::LocalTime));
}

bool sameMinuteBucket(const QDateTime& lhs, const QDateTime& rhs)
{
    return lhs.date() == rhs.date() && lhs.time().hour() == rhs.time().hour() &&
           lhs.time().minute() == rhs.time().minute();
}
} // namespace

bool ChatMessageModel::shouldShowTimeDivider(int row) const
{
    if (row < 0 || row >= rowCount())
    {
        return false;
    }
    if (row == 0)
    {
        return true;
    }

    const QDateTime current = localDateTimeForMs(_items[static_cast<size_t>(row)].createdAt);
    const QDateTime previous = localDateTimeForMs(_items[static_cast<size_t>(row - 1)].createdAt);
    if (current.date() != previous.date())
    {
        return true;
    }
    return current.date() == QDate::currentDate() && !sameMinuteBucket(previous, current);
}

QString ChatMessageModel::timeDividerText(int row) const
{
    if (!shouldShowTimeDivider(row))
    {
        return QString();
    }

    const QDateTime messageDateTime = localDateTimeForMs(_items[static_cast<size_t>(row)].createdAt);
    const QDate currentDate = QDate::currentDate();
    const QDate messageDate = messageDateTime.date();
    if (messageDate == currentDate)
    {
        return messageDateTime.toString(QStringLiteral("HH:mm"));
    }

    const int daysAgo = messageDate.daysTo(currentDate);
    if (daysAgo >= 1 && daysAgo <= 7)
    {
        return QStringLiteral("%1 %2") .arg(weekdayText(messageDate.dayOfWeek()),
                                            messageDateTime.toString(QStringLiteral("HH:mm")));
    }
    if (messageDate.year() == currentDate.year())
    {
        return messageDateTime.toString(QStringLiteral("M月d日 HH:mm"));
    }
    return messageDateTime.toString(QStringLiteral("yyyy年M月d日 HH:mm"));
}

void ChatMessageModel::refreshTimeDividerRange(int firstRow, int lastRow)
{
    if (_items.empty())
    {
        return;
    }

    const int topRow = qMax(0, firstRow);
    const int bottomRow = qMin(rowCount() - 1, lastRow);
    if (topRow > bottomRow)
    {
        return;
    }

    const QModelIndex top = index(topRow, 0);
    const QModelIndex bottom = index(bottomRow, 0);
    emit dataChanged(top, bottom, {ShowTimeDividerRole, TimeDividerTextRole});
}

void ChatMessageModel::restartTimeDividerRefreshTimer()
{
    if (_items.empty())
    {
        _time_divider_refresh_timer.stop();
        return;
    }

    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    const int delayMs = static_cast<int>(60000 - (nowMs % 60000));
    _time_divider_refresh_timer.start(qMax(1, delayMs));
}

void ChatMessageModel::stopTimeDividerRefreshTimer()
{
    _time_divider_refresh_timer.stop();
}
void ChatMessageModel::refreshAvatarFlags()
{
    recomputeAvatarFlags();
    if (!_items.empty())
    {
        const QModelIndex top = index(0, 0);
        const QModelIndex bottom = index(rowCount() - 1, 0);
        emit dataChanged(top, bottom, {ShowAvatarRole});
    }
}

void ChatMessageModel::recomputeAvatarFlags()
{
    const MessageEntry* previous = nullptr;
    for (int row = 0; row < rowCount(); ++row)
    {
        auto& entry = _items[static_cast<size_t>(row)];
        entry.showAvatar = shouldShowTimeDivider(row) || shouldShowAvatarForEntry(previous, entry);
        previous = &entry;
    }
}

int ChatMessageModel::indexOfMessage(const QString& msgId) const
{
    if (msgId.isEmpty())
    {
        return -1;
    }
    for (int i = 0; i < rowCount(); ++i)
    {
        if (_items[static_cast<size_t>(i)].msgId == msgId)
        {
            return i;
        }
    }
    return -1;
}

void ChatMessageModel::refreshAvatarRange(int firstRow, int lastRow)
{
    if (_items.empty())
    {
        return;
    }

    const int topRow = qMax(0, firstRow);
    const int bottomRow = qMin(rowCount() - 1, lastRow);
    if (topRow > bottomRow)
    {
        return;
    }

    emit dataChanged(index(topRow, 0), index(bottomRow, 0), {ShowAvatarRole});
}

void ChatMessageModel::refreshSurroundingRows(int centerRow, const QVector<int>& roles)
{
    if (_items.empty() || centerRow < 0 || centerRow >= rowCount())
    {
        return;
    }

    const int topRow = qMax(0, centerRow - 1);
    const int bottomRow = qMin(rowCount() - 1, centerRow + 1);
    QVector<int> mergedRoles = roles;
    mergedRoles.append(ShowAvatarRole);
    mergedRoles.append(ShowTimeDividerRole);
    mergedRoles.append(TimeDividerTextRole);
    emit dataChanged(index(topRow, 0), index(bottomRow, 0), mergedRoles);
}

int ChatMessageModel::findInsertPosition(const MessageEntry& entry) const
{
    if (_items.empty())
    {
        return 0;
    }

    const auto insertIt = std::upper_bound(_items.begin(),
                                           _items.end(),
                                           entry,
                                           [](const MessageEntry& lhs, const MessageEntry& rhs)
                                           {
                                               return ChatMessageModel::lessThan(lhs, rhs);
                                           });
    return static_cast<int>(std::distance(_items.begin(), insertIt));
}
