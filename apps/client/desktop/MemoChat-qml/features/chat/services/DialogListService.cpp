#include "DialogListService.h"

#include <algorithm>
#include <limits>

#include "DialogListEntryBuilder.h"

namespace
{
int makeGroupDialogUid(qint64 groupId)
{
    if (groupId <= 0 || groupId > static_cast<qint64>(std::numeric_limits<int>::max()))
    {
        return 0;
    }
    return -static_cast<int>(groupId);
}

int resolveGroupDialogUid(QMap<int, qint64>& groupUidMap, qint64 groupId)
{
    if (groupId <= 0)
    {
        return 0;
    }
    for (auto it = groupUidMap.cbegin(); it != groupUidMap.cend(); ++it)
    {
        if (it.value() == groupId)
        {
            return it.key();
        }
    }
    const int dialogUid = makeGroupDialogUid(groupId);
    if (dialogUid == 0)
    {
        return 0;
    }
    groupUidMap.insert(dialogUid, groupId);
    return dialogUid;
}
} // namespace

std::shared_ptr<AuthInfo>
DialogListService::buildPlaceholderAuthInfo(int uid, const QVariantMap& dialogItem, const QString& defaultIcon)
{
    return DialogListEntryBuilder::buildPlaceholderAuthInfo(uid, dialogItem, defaultIcon);
}

std::shared_ptr<FriendInfo> DialogListService::buildDialogEntry(const DialogEntrySeed& seed,
                                                                const DialogDecorationState& state)
{
    return DialogListEntryBuilder::buildDialogEntry(seed, state);
}

QString DialogListService::privateDisplayName(const std::shared_ptr<FriendInfo>& friendInfo)
{
    return DialogListEntryBuilder::privateDisplayName(friendInfo);
}

void DialogListService::applyFriendProfileToPrivateSeed(DialogEntrySeed& seed,
                                                        const std::shared_ptr<FriendInfo>& friendInfo)
{
    DialogListEntryBuilder::applyFriendProfileToPrivateSeed(seed, friendInfo);
}

void DialogListService::appendMissingPrivateDialogs(std::vector<std::shared_ptr<FriendInfo>>& dialogs,
                                                    const std::vector<std::shared_ptr<FriendInfo>>& friends,
                                                    const QSet<int>& existingPrivateUids,
                                                    const DialogDecorationState& state)
{
    QSet<int> knownPrivateUids = existingPrivateUids;
    for (const auto& dialog : dialogs)
    {
        if (dialog && dialog->_uid > 0)
        {
            knownPrivateUids.insert(dialog->_uid);
        }
    }

    for (const auto& friendInfo : friends)
    {
        if (!friendInfo || friendInfo->_uid <= 0 || knownPrivateUids.contains(friendInfo->_uid))
        {
            continue;
        }

        DialogEntrySeed seed;
        seed.dialogUid = friendInfo->_uid;
        seed.dialogType = QStringLiteral("private");
        seed.userId = friendInfo->_user_id;
        seed.name = friendInfo->_name;
        seed.nick = friendInfo->_nick;
        seed.icon = friendInfo->_icon;
        seed.desc = friendInfo->_desc;
        seed.back = friendInfo->_back;
        seed.previewText = friendInfo->_last_msg;
        seed.sex = friendInfo->_sex;
        seed.lastMsgTs = friendInfo->_last_msg_ts;
        if (!friendInfo->_chat_msgs.empty() && friendInfo->_chat_msgs.back())
        {
            seed.lastMsgTs = friendInfo->_chat_msgs.back()->_created_at;
        }
        applyFriendProfileToPrivateSeed(seed, friendInfo);

        auto item = buildDialogEntry(seed, state);
        if (!item)
        {
            continue;
        }
        dialogs.push_back(item);
        knownPrivateUids.insert(friendInfo->_uid);
    }
}

void DialogListService::appendMissingGroupDialogs(std::vector<std::shared_ptr<FriendInfo>>& dialogs,
                                                  const std::vector<std::shared_ptr<GroupInfoData>>& groups,
                                                  QMap<int, qint64>& groupUidMap,
                                                  const QSet<qint64>& existingGroupIds,
                                                  const DialogDecorationState& state)
{
    QSet<qint64> knownGroupIds = existingGroupIds;
    for (const auto& dialog : dialogs)
    {
        if (!dialog || dialog->_uid >= 0)
        {
            continue;
        }
        const qint64 groupId = groupUidMap.value(dialog->_uid, -static_cast<qint64>(dialog->_uid));
        if (groupId > 0)
        {
            knownGroupIds.insert(groupId);
        }
    }

    for (const auto& group : groups)
    {
        if (!group || group->_group_id <= 0 || knownGroupIds.contains(group->_group_id))
        {
            continue;
        }

        const int dialogUid = resolveGroupDialogUid(groupUidMap, group->_group_id);
        if (dialogUid == 0)
        {
            continue;
        }

        DialogEntrySeed seed;
        seed.dialogUid = dialogUid;
        seed.dialogType = QStringLiteral("group");
        seed.name = group->_name;
        seed.nick = group->_name;
        seed.icon = group->_icon.trimmed().isEmpty() ? QStringLiteral("qrc:/res/chat_icon.png") : group->_icon;
        seed.desc = group->_announcement;
        seed.back = group->_announcement;
        seed.previewText = group->_last_msg;
        if (!group->_chat_msgs.empty() && group->_chat_msgs.back())
        {
            seed.lastMsgTs = group->_chat_msgs.back()->_created_at;
        }

        auto item = buildDialogEntry(seed, state);
        if (!item)
        {
            continue;
        }
        dialogs.push_back(item);
        knownGroupIds.insert(group->_group_id);
    }
}

void DialogListService::appendExistingDialogs(std::vector<std::shared_ptr<FriendInfo>>& dialogs,
                                              const std::vector<QVariantMap>& existingDialogs,
                                              const DialogDecorationState& state)
{
    QSet<int> knownUids;
    for (const auto& dialog : dialogs)
    {
        if (dialog && dialog->_uid != 0)
        {
            knownUids.insert(dialog->_uid);
        }
    }

    for (const QVariantMap& existing : existingDialogs)
    {
        const int uid = existing.value(QStringLiteral("uid")).toInt();
        if (uid == 0 || knownUids.contains(uid))
        {
            continue;
        }

        DialogEntrySeed seed;
        seed.dialogUid = uid;
        seed.dialogType = existing.value(QStringLiteral("dialogType")).toString();
        if (seed.dialogType.trimmed().isEmpty())
        {
            seed.dialogType = uid < 0 ? QStringLiteral("group") : QStringLiteral("private");
        }
        seed.userId = existing.value(QStringLiteral("userId")).toString();
        seed.name = existing.value(QStringLiteral("name")).toString();
        seed.nick = existing.value(QStringLiteral("nick")).toString();
        seed.icon = existing.value(QStringLiteral("icon")).toString();
        seed.desc = existing.value(QStringLiteral("desc")).toString();
        seed.back = existing.value(QStringLiteral("back")).toString();
        seed.previewText = existing.value(QStringLiteral("lastMsg")).toString();
        seed.sex = existing.value(QStringLiteral("sex")).toInt();
        seed.unreadCount = existing.value(QStringLiteral("unreadCount")).toInt();
        seed.pinnedRank = existing.value(QStringLiteral("pinnedRank")).toInt();
        seed.draftText = existing.value(QStringLiteral("draftText")).toString();
        seed.lastMsgTs = existing.value(QStringLiteral("lastMsgTs")).toLongLong();
        seed.muteState = existing.value(QStringLiteral("muteState")).toInt();

        auto item = buildDialogEntry(seed, state);
        if (!item)
        {
            continue;
        }
        dialogs.push_back(item);
        knownUids.insert(uid);
    }
}

std::vector<std::shared_ptr<FriendInfo>>
DialogListService::buildSortedSnapshotDialogs(const std::vector<std::shared_ptr<FriendInfo>>& friends,
                                              const std::vector<std::shared_ptr<GroupInfoData>>& groups,
                                              QMap<int, qint64>& groupUidMap,
                                              const DialogDecorationState& state)
{
    std::vector<std::shared_ptr<FriendInfo>> dialogs;
    dialogs.reserve(friends.size() + groups.size());
    appendMissingPrivateDialogs(dialogs, friends, {}, state);
    appendMissingGroupDialogs(dialogs, groups, groupUidMap, {}, state);
    sortDialogs(dialogs);
    return dialogs;
}

void DialogListService::sortDialogs(std::vector<std::shared_ptr<FriendInfo>>& dialogs)
{
    std::stable_sort(dialogs.begin(),
                     dialogs.end(),
                     [](const std::shared_ptr<FriendInfo>& lhs, const std::shared_ptr<FriendInfo>& rhs)
                     {
                         if (!lhs || !rhs)
                         {
                             return static_cast<bool>(lhs);
                         }
                         if (lhs->_pinned_rank != rhs->_pinned_rank)
                         {
                             return lhs->_pinned_rank > rhs->_pinned_rank;
                         }
                         if (lhs->_last_msg_ts != rhs->_last_msg_ts)
                         {
                             return lhs->_last_msg_ts > rhs->_last_msg_ts;
                         }
                         return lhs->_uid < rhs->_uid;
                     });
}
