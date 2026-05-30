#include "DialogListEntryBuilder.h"

#include <QtGlobal>

namespace
{
constexpr int kLocalPinBoost = 1000000;

QString trimmedOrFallback(const QString& value, const QString& fallback)
{
    const QString trimmed = value.trimmed();
    return trimmed.isEmpty() ? fallback : trimmed;
}
} // namespace

std::shared_ptr<AuthInfo>
DialogListEntryBuilder::buildPlaceholderAuthInfo(int uid, const QVariantMap& dialogItem, const QString& defaultIcon)
{
    QString fallbackName = QString::number(uid);
    QString fallbackNick = fallbackName;
    QString fallbackIcon = defaultIcon;

    const QString dialogName = dialogItem.value(QStringLiteral("name")).toString().trimmed();
    const QString dialogNick = dialogItem.value(QStringLiteral("nick")).toString().trimmed();
    const QString dialogIcon = dialogItem.value(QStringLiteral("icon")).toString().trimmed();
    if (!dialogName.isEmpty())
    {
        fallbackName = dialogName;
    }
    fallbackNick = trimmedOrFallback(dialogNick, fallbackName);
    if (!dialogIcon.isEmpty())
    {
        fallbackIcon = dialogIcon;
    }

    return std::make_shared<AuthInfo>(uid, fallbackName, fallbackNick, fallbackIcon, 0, QString());
}

std::shared_ptr<FriendInfo> DialogListEntryBuilder::buildDialogEntry(const DialogEntrySeed& seed,
                                                                     const DialogDecorationState& state)
{
    if (seed.dialogUid == 0)
    {
        return nullptr;
    }

    auto item = std::make_shared<FriendInfo>(seed.dialogUid,
                                             seed.name,
                                             seed.nick,
                                             seed.icon,
                                             seed.sex,
                                             seed.desc,
                                             seed.back,
                                             seed.previewText,
                                             seed.userId);
    item->_dialog_type = seed.dialogType;
    item->_unread_count = qMax(0, seed.unreadCount);
    item->_pinned_rank = qMax(0, seed.pinnedRank);
    item->_draft_text = seed.draftText;
    item->_last_msg_ts = seed.lastMsgTs;
    item->_mute_state = qMax(0, seed.muteState);

    if (state.serverPinnedMap)
    {
        item->_pinned_rank = qMax(item->_pinned_rank, state.serverPinnedMap->value(seed.dialogUid, 0));
    }
    if (state.serverMuteMap)
    {
        item->_mute_state = state.serverMuteMap->value(seed.dialogUid, item->_mute_state);
    }
    if (state.draftMap && state.draftMap->contains(seed.dialogUid))
    {
        item->_draft_text = state.draftMap->value(seed.dialogUid);
    }
    if (state.mentionMap)
    {
        item->_mention_count = qMax(0, state.mentionMap->value(seed.dialogUid, 0));
    }
    if (state.localPinnedSet && state.localPinnedSet->contains(seed.dialogUid))
    {
        item->_pinned_rank = qMax(item->_pinned_rank, kLocalPinBoost);
    }

    return item;
}

QString DialogListEntryBuilder::privateDisplayName(const std::shared_ptr<FriendInfo>& friendInfo)
{
    if (!friendInfo)
    {
        return {};
    }

    const QString back = friendInfo->_back.trimmed();
    if (!back.isEmpty())
    {
        return back;
    }
    const QString nick = friendInfo->_nick.trimmed();
    if (!nick.isEmpty())
    {
        return nick;
    }
    const QString name = friendInfo->_name.trimmed();
    if (!name.isEmpty())
    {
        return name;
    }
    return friendInfo->_uid > 0 ? QString::number(friendInfo->_uid) : QString();
}

void DialogListEntryBuilder::applyFriendProfileToPrivateSeed(DialogEntrySeed& seed,
                                                             const std::shared_ptr<FriendInfo>& friendInfo)
{
    if (!friendInfo || friendInfo->_uid <= 0 || seed.dialogUid != friendInfo->_uid)
    {
        return;
    }

    const QString displayName = privateDisplayName(friendInfo);
    if (!displayName.isEmpty())
    {
        seed.name = displayName;
    }
    const QString nick = friendInfo->_nick.trimmed();
    seed.nick = nick.isEmpty() ? seed.name : nick;
    seed.userId = friendInfo->_user_id;
    seed.desc = friendInfo->_desc;
    seed.back = friendInfo->_back;
    seed.sex = friendInfo->_sex;
    if (!friendInfo->_icon.trimmed().isEmpty())
    {
        seed.icon = friendInfo->_icon;
    }
    if (seed.previewText.trimmed().isEmpty())
    {
        seed.previewText = friendInfo->_last_msg;
    }
    if (seed.lastMsgTs <= 0)
    {
        seed.lastMsgTs = friendInfo->_last_msg_ts;
        if (!friendInfo->_chat_msgs.empty() && friendInfo->_chat_msgs.back())
        {
            seed.lastMsgTs = friendInfo->_chat_msgs.back()->_created_at;
        }
    }
}
