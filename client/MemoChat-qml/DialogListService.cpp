#include "DialogListService.h"

#include <QtGlobal>
#include <algorithm>

namespace {
constexpr int kLocalPinBoost = 1000000;

QString trimmedOrFallback(const QString &value, const QString &fallback)
{
    const QString trimmed = value.trimmed();
    return trimmed.isEmpty() ? fallback : trimmed;
}
}

std::shared_ptr<AuthInfo> DialogListService::buildPlaceholderAuthInfo(int uid,
                                                                      const QVariantMap &dialogItem,
                                                                      const QString &defaultIcon)
{
    QString fallbackName = QString::number(uid);
    QString fallbackNick = fallbackName;
    QString fallbackIcon = defaultIcon;

    const QString dialogName = dialogItem.value(QStringLiteral("name")).toString().trimmed();
    const QString dialogNick = dialogItem.value(QStringLiteral("nick")).toString().trimmed();
    const QString dialogIcon = dialogItem.value(QStringLiteral("icon")).toString().trimmed();
    if (!dialogName.isEmpty()) {
        fallbackName = dialogName;
    }
    fallbackNick = trimmedOrFallback(dialogNick, fallbackName);
    if (!dialogIcon.isEmpty()) {
        fallbackIcon = dialogIcon;
    }

    return std::make_shared<AuthInfo>(uid, fallbackName, fallbackNick, fallbackIcon, 0, QString());
}

std::shared_ptr<FriendInfo> DialogListService::buildDialogEntry(const DialogEntrySeed &seed,
                                                                const DialogDecorationState &state)
{
    if (seed.dialogUid == 0) {
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

    if (state.serverPinnedMap) {
        item->_pinned_rank = qMax(item->_pinned_rank, state.serverPinnedMap->value(seed.dialogUid, 0));
    }
    if (state.serverMuteMap) {
        item->_mute_state = state.serverMuteMap->value(seed.dialogUid, item->_mute_state);
    }
    if (state.draftMap && state.draftMap->contains(seed.dialogUid)) {
        item->_draft_text = state.draftMap->value(seed.dialogUid);
    }
    if (state.mentionMap) {
        item->_mention_count = qMax(0, state.mentionMap->value(seed.dialogUid, 0));
    }
    if (state.localPinnedSet && state.localPinnedSet->contains(seed.dialogUid)) {
        item->_pinned_rank = qMax(item->_pinned_rank, kLocalPinBoost);
    }

    return item;
}

void DialogListService::sortDialogs(std::vector<std::shared_ptr<FriendInfo>> &dialogs)
{
    std::stable_sort(dialogs.begin(), dialogs.end(),
                     [](const std::shared_ptr<FriendInfo> &lhs, const std::shared_ptr<FriendInfo> &rhs) {
                         if (!lhs || !rhs) {
                             return static_cast<bool>(lhs);
                         }
                         if (lhs->_pinned_rank != rhs->_pinned_rank) {
                             return lhs->_pinned_rank > rhs->_pinned_rank;
                         }
                         if (lhs->_last_msg_ts != rhs->_last_msg_ts) {
                             return lhs->_last_msg_ts > rhs->_last_msg_ts;
                         }
                         return lhs->_uid < rhs->_uid;
                     });
}
