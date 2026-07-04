#include "FriendListModel.h"

#include <algorithm>

namespace
{
QString nonEmptyOrExisting(const QString& next, const QString& existing)
{
    return next.trimmed().isEmpty() ? existing : next;
}

int sectionRank(const QString& sectionKey)
{
    if (sectionKey == QStringLiteral("#"))
    {
        return 26;
    }
    if (sectionKey.size() == 1)
    {
        const ushort code = sectionKey.front().unicode();
        if (code >= 'A' && code <= 'Z')
        {
            return static_cast<int>(code - 'A');
        }
    }
    return 27;
}
} // namespace

void FriendListModel::upsert(const FriendEntry& entry)
{
    FriendEntry nextEntry = entry;
    refreshDerivedFields(nextEntry);

    const int idx = indexOfUid(entry.uid);
    if (_contact_section_ordering_enabled)
    {
        const int oldCount = rowCount();
        std::vector<FriendEntry> nextItems = _items;
        if (idx >= 0)
        {
            nextItems[static_cast<size_t>(idx)] = mergeSparseEntry(nextEntry, nextItems[static_cast<size_t>(idx)]);
        }
        else
        {
            nextItems.push_back(nextEntry);
        }
        sortForContactSections(nextItems);

        beginResetModel();
        _items = std::move(nextItems);
        endResetModel();
        if (rowCount() != oldCount)
        {
            emit countChanged();
        }
        return;
    }

    if (idx >= 0)
    {
        FriendEntry& stored = _items[static_cast<size_t>(idx)];
        stored = mergeSparseEntry(nextEntry, stored);
        const QModelIndex modelIndex = index(idx, 0);
        emit dataChanged(modelIndex, modelIndex);
        return;
    }

    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    _items.push_back(nextEntry);
    endInsertRows();
    emit countChanged();
}

FriendListModel::FriendEntry FriendListModel::mergeSparseEntry(const FriendEntry& entry, const FriendEntry& existing)
{
    FriendEntry merged = existing;
    merged.uid = entry.uid;
    merged.userId = nonEmptyOrExisting(entry.userId, existing.userId);
    merged.name = nonEmptyOrExisting(entry.name, existing.name);
    merged.nick = nonEmptyOrExisting(entry.nick, existing.nick);
    merged.icon = nonEmptyOrExisting(entry.icon, existing.icon);
    merged.desc = nonEmptyOrExisting(entry.desc, existing.desc);
    merged.lastMsg = nonEmptyOrExisting(entry.lastMsg, existing.lastMsg);
    merged.back = nonEmptyOrExisting(entry.back, existing.back);
    merged.dialogType = nonEmptyOrExisting(entry.dialogType, existing.dialogType);
    if (entry.sex != 0)
    {
        merged.sex = entry.sex;
    }
    merged.unreadCount = entry.unreadCount;
    merged.pinnedRank = entry.pinnedRank;
    merged.draftText = entry.draftText;
    merged.lastMsgTs = entry.lastMsgTs;
    merged.muteState = entry.muteState;
    merged.mentionCount = entry.mentionCount;
    refreshDerivedFields(merged);
    return merged;
}

QString FriendListModel::displayNameForSection(const FriendEntry& entry)
{
    for (const QString& value : {entry.name, entry.nick, entry.back, entry.userId})
    {
        const QString trimmed = value.trimmed();
        if (!trimmed.isEmpty())
        {
            return trimmed;
        }
    }
    return entry.uid > 0 ? QString::number(entry.uid) : QString();
}

QString FriendListModel::sectionKeyForEntry(const FriendEntry& entry)
{
    const QString displayName = displayNameForSection(entry);
    if (displayName.isEmpty())
    {
        return QStringLiteral("#");
    }

    const ushort code = displayName.front().unicode();
    if (code >= 'a' && code <= 'z')
    {
        return QString(QChar(static_cast<ushort>(code - ('a' - 'A'))));
    }
    if (code >= 'A' && code <= 'Z')
    {
        return QString(QChar(code));
    }
    return QStringLiteral("#");
}

bool FriendListModel::contactSectionLessThan(const FriendEntry& lhs, const FriendEntry& rhs)
{
    const int lhsRank = sectionRank(lhs.sectionKey);
    const int rhsRank = sectionRank(rhs.sectionKey);
    if (lhsRank != rhsRank)
    {
        return lhsRank < rhsRank;
    }

    const QString lhsName = displayNameForSection(lhs);
    const QString rhsName = displayNameForSection(rhs);
    const int caseInsensitive = QString::compare(lhsName, rhsName, Qt::CaseInsensitive);
    if (caseInsensitive != 0)
    {
        return caseInsensitive < 0;
    }

    const int caseSensitive = QString::compare(lhsName, rhsName, Qt::CaseSensitive);
    if (caseSensitive != 0)
    {
        return caseSensitive < 0;
    }
    return lhs.uid < rhs.uid;
}

void FriendListModel::refreshDerivedFields(FriendEntry& entry)
{
    entry.sectionKey = sectionKeyForEntry(entry);
}

void FriendListModel::sortForContactSections(std::vector<FriendEntry>& items)
{
    for (FriendEntry& entry : items)
    {
        refreshDerivedFields(entry);
    }
    std::stable_sort(items.begin(), items.end(), contactSectionLessThan);
}
