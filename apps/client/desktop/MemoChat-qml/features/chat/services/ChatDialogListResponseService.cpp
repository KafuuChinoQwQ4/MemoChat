#include "ChatDialogListResponseService.h"

#include "ConversationSyncService.h"
#include "DialogListService.h"
#include "userdata.h"

#include <QJsonArray>
#include <utility>

namespace
{
constexpr int kSuccessErrorCode = 0;
constexpr int kJsonErrorCode = 1;

qint64 normalizeLastMessageTs(qint64 value)
{
    if (value > 0 && value < 100000000000LL)
    {
        return value * 1000;
    }
    return value;
}

std::vector<std::shared_ptr<FriendInfo>> buildFallbackDialogs(const ChatDialogListResponseContext& context,
                                                              QMap<int, qint64>& groupDialogUidMap)
{
    std::vector<std::shared_ptr<FriendInfo>> merged;
    DialogListService::appendMissingPrivateDialogs(merged, context.friendSnapshot, {}, context.decorationState);
    DialogListService::appendMissingGroupDialogs(merged,
                                                 context.groupSnapshot,
                                                 groupDialogUidMap,
                                                 {},
                                                 context.decorationState);
    DialogListService::sortDialogs(merged);
    return merged;
}

template <typename Callback, typename... Args> void callIfPresent(const Callback& callback, Args&&... args)
{
    if (callback)
    {
        callback(std::forward<Args>(args)...);
    }
}
} // namespace

ChatDialogListResponseEffect ChatDialogListResponseService::reduce(const QJsonObject& payload,
                                                                   ChatDialogListResponseContext context,
                                                                   QMap<int, qint64>& groupDialogUidMap)
{
    ChatDialogListResponseEffect effect;
    effect.dialogsReady = true;
    effect.chatListInitialized = true;
    effect.flushIncomingMessages = true;

    if (!payload.contains(QStringLiteral("dialogs")) ||
                          payload.value(QStringLiteral("error")).toInt(kJsonErrorCode) != kSuccessErrorCode)
    {
        if (context.bootstrappingDialog && !context.hasCurrentDialog)
        {
            effect.selectFirstChat = true;
        }
        return effect;
    }

    const QJsonArray dialogs = payload.value(QStringLiteral("dialogs")).toArray();
    std::vector<std::shared_ptr<FriendInfo>> merged;
    merged.reserve(dialogs.size());
    effect.resetServerDialogMeta = true;

    for (const auto& one : dialogs)
    {
        const QJsonObject obj = one.toObject();
        const QString dialogType = obj.value(QStringLiteral("dialog_type")).toString();
        const QString title = obj.value(QStringLiteral("title")).toString();
        const QString avatar = obj.value(QStringLiteral("avatar")).toString();
        const QString preview = obj.value(QStringLiteral("last_msg_preview")).toString();

        if (dialogType == QStringLiteral("private"))
        {
            const int peerUid = obj.value(QStringLiteral("peer_uid")).toInt();
            if (peerUid <= 0)
            {
                continue;
            }
            DialogEntrySeed seed;
            seed.dialogUid = peerUid;
            seed.dialogType = dialogType;
            seed.name = title;
            seed.nick = title;
            seed.icon = avatar;
            seed.previewText = preview;
            seed.unreadCount = obj.value(QStringLiteral("unread_count")).toInt(0);
            seed.pinnedRank = obj.value(QStringLiteral("pinned_rank")).toInt(0);
            seed.draftText = obj.value(QStringLiteral("draft_text")).toString();
            seed.lastMsgTs = normalizeLastMessageTs(obj.value(QStringLiteral("last_msg_ts")).toVariant().toLongLong());
            seed.muteState = obj.value(QStringLiteral("mute_state")).toInt(0);
            effect.privateDialogUids.insert(peerUid);
            effect.serverMetaSeeds.push_back(ChatDialogServerMetaSeed{peerUid, seed.pinnedRank, seed.muteState});
            for (const auto& friendInfo : context.friendSnapshot)
            {
                if (friendInfo && friendInfo->_uid == peerUid)
                {
                    DialogListService::applyFriendProfileToPrivateSeed(seed, friendInfo);
                    break;
                }
            }
            auto item = DialogListService::buildDialogEntry(seed, context.decorationState);
            if (item)
            {
                merged.push_back(item);
            }
            continue;
        }

        if (dialogType == QStringLiteral("group"))
        {
            const qint64 groupId = obj.value(QStringLiteral("group_id")).toVariant().toLongLong();
            if (groupId <= 0)
            {
                continue;
            }
            const int pseudoUid = ConversationSyncService::resolveGroupDialogUid(groupDialogUidMap, groupId);
            if (pseudoUid == 0)
            {
                continue;
            }
            const QString groupIcon = avatar.trimmed().isEmpty() ? QStringLiteral("qrc:/res/chat_icon.png") : avatar;
            DialogEntrySeed seed;
            seed.dialogUid = pseudoUid;
            seed.dialogType = dialogType;
            seed.name = title;
            seed.nick = title;
            seed.icon = groupIcon;
            seed.previewText = preview;
            seed.unreadCount = obj.value(QStringLiteral("unread_count")).toInt(0);
            seed.pinnedRank = obj.value(QStringLiteral("pinned_rank")).toInt(0);
            seed.draftText = obj.value(QStringLiteral("draft_text")).toString();
            seed.lastMsgTs = normalizeLastMessageTs(obj.value(QStringLiteral("last_msg_ts")).toVariant().toLongLong());
            seed.muteState = obj.value(QStringLiteral("mute_state")).toInt(0);
            auto groupInfo = std::make_shared<GroupInfoData>();
            groupInfo->_group_id = groupId;
            groupInfo->_name = title;
            groupInfo->_icon = groupIcon;
            groupInfo->_last_msg = preview;
            effect.upsertGroups.push_back(groupInfo);
            effect.groupDialogIds.insert(groupId);
            effect.serverMetaSeeds.push_back(ChatDialogServerMetaSeed{pseudoUid, seed.pinnedRank, seed.muteState});
            auto item = DialogListService::buildDialogEntry(seed, context.decorationState);
            if (item)
            {
                merged.push_back(item);
            }
            groupDialogUidMap.insert(pseudoUid, groupId);
        }
    }

    DialogListService::appendMissingPrivateDialogs(merged,
                                                   context.friendSnapshot,
                                                   effect.privateDialogUids,
                                                   context.decorationState);
    DialogListService::appendMissingGroupDialogs(merged,
                                                 context.groupSnapshot,
                                                 groupDialogUidMap,
                                                 effect.groupDialogIds,
                                                 context.decorationState);
    DialogListService::appendExistingDialogs(merged, context.existingDialogs, context.decorationState);

    for (const auto& dialog : merged)
    {
        if (dialog && dialog->_uid > 0)
        {
            effect.upsertChatDialogs.push_back(dialog);
        }
        else if (dialog && dialog->_uid < 0)
        {
            effect.upsertGroupDialogs.push_back(dialog);
        }
    }

    if (merged.empty() && (!context.friendSnapshot.empty() || !context.groupSnapshot.empty()))
    {
        effect.refreshDialogModel = true;
        if (context.bootstrappingDialog && !context.hasCurrentDialog)
        {
            const auto fallback = buildFallbackDialogs(context, groupDialogUidMap);
            if (!fallback.empty() && fallback.front())
            {
                effect.selectDialogUid = fallback.front()->_uid;
            }
        }
        return effect;
    }

    DialogListService::sortDialogs(merged);
    effect.replaceDialogList = merged;
    if (context.currentGroupId > 0)
    {
        const int dialogUid = ConversationSyncService::resolveGroupDialogUid(groupDialogUidMap, context.currentGroupId);
        effect.clearUnreadDialogUids.push_back(dialogUid);
        effect.clearMentionDialogUids.push_back(dialogUid);
    }
    else if (context.currentPrivatePeerUid > 0)
    {
        effect.clearUnreadDialogUids.push_back(context.currentPrivatePeerUid);
        effect.clearMentionDialogUids.push_back(context.currentPrivatePeerUid);
    }
    effect.syncCurrentDialogDraft = true;

    const bool shouldSelectDialog = context.bootstrappingDialog || !context.hasCurrentDialog;
    if (shouldSelectDialog)
    {
        if (!merged.empty() && merged.front())
        {
            effect.selectDialogUid = merged.front()->_uid;
            effect.clearUnreadDialogUids.push_back(effect.selectDialogUid);
            effect.clearMentionDialogUids.push_back(effect.selectDialogUid);
        }
        else
        {
            effect.selectFirstChat = true;
        }
    }

    if (context.bootstrappingDialog)
    {
        for (const auto& dialog : merged)
        {
            if (!dialog || dialog->_unread_count <= 0)
            {
                continue;
            }
            if (dialog->_uid > 0)
            {
                if (dialog->_uid == context.currentPrivatePeerUid)
                {
                    effect.currentPrivateHistoryUid = dialog->_uid;
                }
                else
                {
                    effect.bootstrapPrivateHistoryUids.push_back(dialog->_uid);
                }
            }
            else if (dialog->_uid < 0)
            {
                const qint64 groupId = ConversationSyncService::groupIdForDialogUid(groupDialogUidMap, dialog->_uid);
                if (groupId > 0)
                {
                    effect.bootstrapGroupHistoryIds.push_back(groupId);
                }
            }
        }
    }
    return effect;
}

void ChatDialogListResponseService::apply(const ChatDialogListResponseEffect& effect,
                                          const ChatDialogListResponsePort& featurePort,
                                          const ChatDialogListAppPort& appPort)
{
    callIfPresent(appPort.setDialogBootstrapLoading, false);
    if (effect.dialogsReady)
    {
        callIfPresent(appPort.setDialogsReady, true);
    }
    if (effect.chatListInitialized)
    {
        callIfPresent(appPort.setChatListInitialized, true);
    }
    if (effect.resetServerDialogMeta)
    {
        callIfPresent(featurePort.resetServerDialogMeta);
    }
    for (const ChatDialogServerMetaSeed& seed : effect.serverMetaSeeds)
    {
        callIfPresent(featurePort.seedServerDialogMeta, seed.dialogUid, seed.pinnedRank, seed.muteState);
    }
    for (const auto& group : effect.upsertGroups)
    {
        callIfPresent(appPort.upsertGroup, group);
    }
    for (const auto& dialog : effect.upsertChatDialogs)
    {
        callIfPresent(featurePort.upsertChatDialog, dialog);
    }
    for (const auto& dialog : effect.upsertGroupDialogs)
    {
        callIfPresent(appPort.upsertGroupDialog, dialog);
    }
    if (effect.refreshDialogModel)
    {
        callIfPresent(appPort.refreshDialogModel);
    }
    if (!effect.replaceDialogList.empty())
    {
        callIfPresent(featurePort.replaceDialogList, effect.replaceDialogList);
    }
    for (const int dialogUid : effect.clearUnreadDialogUids)
    {
        callIfPresent(featurePort.clearUnreadDialog, dialogUid);
    }
    for (const int dialogUid : effect.clearMentionDialogUids)
    {
        callIfPresent(featurePort.clearMentionDialog, dialogUid);
        callIfPresent(featurePort.removeMentionForDialog, dialogUid);
    }
    if (effect.syncCurrentDialogDraft)
    {
        callIfPresent(appPort.syncCurrentDialogDraft);
    }
    if (effect.selectDialogUid != 0)
    {
        callIfPresent(appPort.selectDialogByUid, effect.selectDialogUid);
    }
    else if (effect.selectFirstChat)
    {
        callIfPresent(appPort.selectFirstChat);
    }
    for (const int peerUid : effect.bootstrapPrivateHistoryUids)
    {
        callIfPresent(appPort.requestPrivateHistoryForBootstrap, peerUid);
    }
    if (effect.currentPrivateHistoryUid > 0)
    {
        callIfPresent(appPort.requestCurrentPrivateHistory, effect.currentPrivateHistoryUid);
    }
    for (const qint64 groupId : effect.bootstrapGroupHistoryIds)
    {
        callIfPresent(appPort.requestGroupHistoryForBootstrap, groupId);
    }
    if (effect.flushIncomingMessages)
    {
        callIfPresent(appPort.flushIncomingMessages);
    }
}

ChatDialogListResponseEffect ChatDialogListResponseService::handle(const QJsonObject& payload,
                                                                   ChatDialogListResponseContext context,
                                                                   QMap<int, qint64>& groupDialogUidMap,
                                                                   const ChatDialogListResponsePort& featurePort,
                                                                   const ChatDialogListAppPort& appPort)
{
    const ChatDialogListResponseEffect effect = reduce(payload, std::move(context), groupDialogUidMap);
    apply(effect, featurePort, appPort);
    return effect;
}
