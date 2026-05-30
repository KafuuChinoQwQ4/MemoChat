#include "AppController.h"
#include "ConversationSyncService.h"
#include "DialogListService.h"
#include "usermgr.h"

#include <QDateTime>
#include <QDebug>

void AppController::refreshDialogModel()
{
    _dialog_list_model.clear();
    std::vector<std::shared_ptr<FriendInfo>> dialogs;
    const DialogDecorationState decorationState{&_dialog_state.draftMap,
                                                &_dialog_state.mentionMap,
                                                &_dialog_state.serverPinnedMap,
                                                &_dialog_state.serverMuteMap,
                                                &_dialog_state.localPinnedSet};

    const auto chats = _gateway.userMgr()->GetFriendListSnapshot();
    dialogs.reserve(chats.size() + _group_state.dialogUidMap.size());
    for (const auto& chat : chats)
    {
        if (!chat)
        {
            continue;
        }

        DialogEntrySeed seed;
        seed.dialogUid = chat->_uid;
        seed.dialogType = QStringLiteral("private");
        seed.userId = chat->_user_id;
        seed.name = chat->_name;
        seed.nick = chat->_nick;
        seed.icon = chat->_icon;
        seed.desc = chat->_desc;
        seed.back = chat->_back;
        seed.previewText = chat->_last_msg;
        seed.sex = chat->_sex;
        if (!chat->_chat_msgs.empty() && chat->_chat_msgs.back())
        {
            seed.lastMsgTs = chat->_chat_msgs.back()->_created_at;
        }
        dialogs.push_back(DialogListService::buildDialogEntry(seed, decorationState));
    }

    const auto groups = _gateway.userMgr()->GetGroupListSnapshot();
    for (const auto& group : groups)
    {
        if (!group || group->_group_id <= 0)
        {
            continue;
        }

        const int pseudoUid =
            ConversationSyncService::resolveGroupDialogUid(_group_state.dialogUidMap, group->_group_id);
        if (pseudoUid == 0)
        {
            continue;
        }

        DialogEntrySeed seed;
        seed.dialogUid = pseudoUid;
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
        dialogs.push_back(DialogListService::buildDialogEntry(seed, decorationState));
    }

    DialogListService::sortDialogs(dialogs);
    _dialog_list_model.upsertBatch(dialogs, true);
    syncCurrentDialogDraft();
}

void AppController::refreshDialogModelIncremental()
{
    const qint64 startTs = QDateTime::currentMSecsSinceEpoch();

    std::vector<std::shared_ptr<FriendInfo>> updates;
    const DialogDecorationState decorationState{&_dialog_state.draftMap,
                                                &_dialog_state.mentionMap,
                                                &_dialog_state.serverPinnedMap,
                                                &_dialog_state.serverMuteMap,
                                                &_dialog_state.localPinnedSet};

    const auto chats = _gateway.userMgr()->GetFriendListSnapshot();
    updates.reserve(chats.size() + 1);
    for (const auto& chat : chats)
    {
        if (!chat)
        {
            continue;
        }

        DialogEntrySeed seed;
        seed.dialogUid = chat->_uid;
        seed.dialogType = QStringLiteral("private");
        seed.userId = chat->_user_id;
        seed.name = chat->_name;
        seed.nick = chat->_nick;
        seed.icon = chat->_icon;
        seed.desc = chat->_desc;
        seed.back = chat->_back;
        seed.previewText = chat->_last_msg;
        seed.sex = chat->_sex;
        if (!chat->_chat_msgs.empty() && chat->_chat_msgs.back())
        {
            seed.lastMsgTs = chat->_chat_msgs.back()->_created_at;
        }
        auto item = DialogListService::buildDialogEntry(seed, decorationState);
        updates.push_back(item);
        _chat_list_model.upsertFriend(item);
    }

    const auto groups = _gateway.userMgr()->GetGroupListSnapshot();
    for (const auto& group : groups)
    {
        if (!group || group->_group_id <= 0)
        {
            continue;
        }

        const int pseudoUid =
            ConversationSyncService::resolveGroupDialogUid(_group_state.dialogUidMap, group->_group_id);
        if (pseudoUid == 0)
        {
            continue;
        }

        DialogEntrySeed seed;
        seed.dialogUid = pseudoUid;
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
        updates.push_back(DialogListService::buildDialogEntry(seed, decorationState));
    }

    _dialog_list_model.upsertBatch(updates);

    syncCurrentDialogDraft();

    const qint64 endTs = QDateTime::currentMSecsSinceEpoch();
    qInfo() << "[PERF] refreshDialogModelIncremental - chats:" << chats.size() << "| groups:" << groups.size()
            << "| total:" << (endTs - startTs) << "ms";
}
