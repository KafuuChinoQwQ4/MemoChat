#include "FriendListModel.h"
#include "IconPathUtils.h"

QVariantMap FriendListModel::get(int indexValue) const
{
    if (indexValue < 0 || indexValue >= rowCount())
    {
        return {};
    }

    const FriendEntry& entry = _items[static_cast<size_t>(indexValue)];
    return {{"uid", entry.uid},
            {"userId", entry.userId},
            {"name", entry.name},
            {"nick", entry.nick},
            {"icon", entry.icon},
            {"desc", entry.desc},
            {"lastMsg", entry.lastMsg},
            {"sex", entry.sex},
            {"back", entry.back},
            {"dialogType", entry.dialogType},
            {"unreadCount", entry.unreadCount},
            {"pinnedRank", entry.pinnedRank},
            {"draftText", entry.draftText},
            {"lastMsgTs", entry.lastMsgTs},
            {"muteState", entry.muteState},
            {"mentionCount", entry.mentionCount},
            {"sectionKey", entry.sectionKey}};
}

int FriendListModel::indexOfUid(int uid) const
{
    for (int i = 0; i < rowCount(); ++i)
    {
        if (_items[static_cast<size_t>(i)].uid == uid)
        {
            return i;
        }
    }

    return -1;
}

QString FriendListModel::normalizeIcon(QString icon)
{
    return normalizeIconForQml(icon);
}
