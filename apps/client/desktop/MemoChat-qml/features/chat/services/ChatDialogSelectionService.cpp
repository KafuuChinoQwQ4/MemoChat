#include "ChatDialogSelectionService.h"

#include "ConversationSyncService.h"
#include "DialogListService.h"
#include "FriendListModel.h"
#include "userdata.h"

#include <QDebug>

namespace
{
QString defaultPrivateIcon()
{
    return QStringLiteral("qrc:/res/head_1.png");
}

QString defaultGroupIcon()
{
    return QStringLiteral("qrc:/res/chat_icon.png");
}

void clearCurrentProjection(const ChatDialogSelectionPort& port, bool clearPrivateHistory = true)
{
    if (port.setPendingReplyContext)
    {
        port.setPendingReplyContext(QString(), QString(), QString());
    }
    if (port.setCurrentPrivatePeerUid)
    {
        port.setCurrentPrivatePeerUid(0);
    }
    if (port.setCurrentGroup)
    {
        port.setCurrentGroup(0, QString(), QString());
    }
    if (port.setCurrentChatPeerName)
    {
        port.setCurrentChatPeerName(QString());
    }
    if (port.setCurrentChatPeerIcon)
    {
        port.setCurrentChatPeerIcon(defaultPrivateIcon());
    }
    if (port.clearMessageModel)
    {
        port.clearMessageModel();
    }
    if (clearPrivateHistory && port.clearPrivateHistoryState)
    {
        port.clearPrivateHistoryState();
    }
    if (port.resetGroupConversationState)
    {
        port.resetGroupConversationState();
    }
    if (port.setGroupHistoryLoading)
    {
        port.setGroupHistoryLoading(false);
    }
    if (port.setPrivateHistoryLoading)
    {
        port.setPrivateHistoryLoading(false);
    }
    if (port.setCanLoadMorePrivateHistory)
    {
        port.setCanLoadMorePrivateHistory(false);
    }
    if (port.emitCurrentDialogUidChangedIfNeeded)
    {
        port.emitCurrentDialogUidChangedIfNeeded();
    }
    if (port.syncCurrentDialogDraft)
    {
        port.syncCurrentDialogDraft();
    }
}

QString normalizedPrivateIcon(const QString& icon)
{
    return icon.trimmed().isEmpty() ? defaultPrivateIcon() : icon;
}

QString normalizedGroupIcon(const QString& icon)
{
    return icon.trimmed().isEmpty() ? defaultGroupIcon() : icon;
}

QVariantMap dialogItemForUid(const FriendListModel* dialogListModel, int uid)
{
    if (!dialogListModel)
    {
        return {};
    }
    const int dialogIndex = dialogListModel->indexOfUid(uid);
    return dialogIndex >= 0 ? dialogListModel->get(dialogIndex) : QVariantMap();
}

QVariantMap groupDialogByIndex(const ChatDialogSelectionPort& port, int index)
{
    return port.groupDialogs.groupDialogByIndex ? port.groupDialogs.groupDialogByIndex(index) : QVariantMap();
}

int indexOfGroupDialog(const ChatDialogSelectionPort& port, int dialogUid)
{
    return port.groupDialogs.indexOfGroupDialog ? port.groupDialogs.indexOfGroupDialog(dialogUid) : -1;
}

QVariantMap groupDialogItem(const ChatDialogSelectionPort& port, int dialogUid)
{
    return port.groupDialogs.groupDialogItem ? port.groupDialogs.groupDialogItem(dialogUid) : QVariantMap();
}

qint64 groupIdForDialogUid(const ChatDialogSelectionPort& port, int dialogUid)
{
    return port.groupDialogs.groupIdForDialogUid ? port.groupDialogs.groupIdForDialogUid(dialogUid) : 0;
}

void clearGroupUnread(const ChatDialogSelectionPort& port, int dialogUid)
{
    if (port.groupDialogs.clearGroupUnread)
    {
        port.groupDialogs.clearGroupUnread(dialogUid);
    }
}

void openCurrentGroupConversation(qint64 groupId, const ChatDialogSelectionPort& port)
{
    if (port.openGroupConversation)
    {
        const qint64 requestedReadAckTs = port.openGroupConversation(groupId);
        if (requestedReadAckTs > 0 && port.sendGroupReadAck)
        {
            port.sendGroupReadAck(groupId, requestedReadAckTs);
        }
    }
    if (port.loadGroupHistory)
    {
        port.loadGroupHistory();
    }
    if (port.syncCurrentDialogDraft)
    {
        port.syncCurrentDialogDraft();
    }
}
} // namespace

void ChatDialogSelectionService::selectPrivateByIndex(int index, const ChatDialogSelectionPort& port)
{
    const QVariantMap item = port.chatListModel ? port.chatListModel->get(index) : QVariantMap();
    if (item.isEmpty())
    {
        clearCurrentProjection(port);
        return;
    }

    const int uid = item.value(QStringLiteral("uid")).toInt();
    if (port.setPendingReplyContext)
    {
        port.setPendingReplyContext(QString(), QString(), QString());
    }
    if (port.setCurrentPrivatePeerUid)
    {
        port.setCurrentPrivatePeerUid(uid);
    }
    if (port.dialogListModel)
    {
        port.dialogListModel->clearUnread(uid);
    }
    if (port.chatListModel)
    {
        port.chatListModel->clearUnread(uid);
    }
    if (port.setCurrentGroup)
    {
        port.setCurrentGroup(0, QString(), QString());
    }
    if (port.resetGroupConversationState)
    {
        port.resetGroupConversationState();
    }
    if (port.setGroupHistoryLoading)
    {
        port.setGroupHistoryLoading(false);
    }

    qInfo() << "Selecting private chat by index:" << index << "uid:" << uid
            << "name:" << item.value(QStringLiteral("name")).toString();
    const auto friendInfo = port.friendById ? port.friendById(uid) : nullptr;
    if (friendInfo)
    {
        if (port.chatListModel)
        {
            port.chatListModel->upsertFriend(friendInfo);
        }
        if (port.setCurrentChatPeerName)
        {
            port.setCurrentChatPeerName(DialogListService::privateDisplayName(friendInfo));
        }
        if (port.setCurrentChatPeerIcon)
        {
            port.setCurrentChatPeerIcon(normalizedPrivateIcon(friendInfo->_icon));
        }
    }
    else
    {
        if (port.setCurrentChatPeerName)
        {
            port.setCurrentChatPeerName(item.value(QStringLiteral("name")).toString());
        }
        if (port.setCurrentChatPeerIcon)
        {
            port.setCurrentChatPeerIcon(normalizedPrivateIcon(item.value(QStringLiteral("icon")).toString()));
        }
    }
    if (port.emitCurrentDialogUidChangedIfNeeded)
    {
        port.emitCurrentDialogUidChangedIfNeeded();
    }
    if (port.loadCurrentPrivateMessages)
    {
        port.loadCurrentPrivateMessages();
    }
    if (port.syncCurrentDialogDraft)
    {
        port.syncCurrentDialogDraft();
    }
}

void ChatDialogSelectionService::selectPrivateByUid(int uid, const ChatDialogSelectionPort& port)
{
    qInfo() << "Selecting private chat by uid:" << uid;
    auto friendInfo = port.friendById ? port.friendById(uid) : nullptr;
    if (!friendInfo)
    {
        const QVariantMap dialogItem = dialogItemForUid(port.dialogListModel, uid);
        auto placeholder = DialogListService::buildPlaceholderAuthInfo(uid, dialogItem, defaultPrivateIcon());
        if (port.addFriend)
        {
            port.addFriend(placeholder);
        }
        if (port.chatListModel)
        {
            port.chatListModel->upsertFriend(placeholder);
        }
        if (port.upsertContact)
        {
            port.upsertContact(placeholder);
        }
        if (port.dialogListModel)
        {
            port.dialogListModel->upsertFriend(placeholder);
        }
        friendInfo = port.friendById ? port.friendById(uid) : nullptr;
        if (!friendInfo)
        {
            return;
        }
    }

    const QVariantMap dialogItem = dialogItemForUid(port.dialogListModel, uid);
    DialogEntrySeed seed;
    seed.dialogUid = uid;
    seed.dialogType = QStringLiteral("private");
    seed.userId = friendInfo->_user_id;
    seed.name = dialogItem.value(QStringLiteral("name"), friendInfo->_name).toString();
    seed.nick = dialogItem.value(QStringLiteral("nick"), friendInfo->_nick).toString();
    seed.icon = dialogItem.value(QStringLiteral("icon"), friendInfo->_icon).toString();
    seed.desc = friendInfo->_desc;
    seed.back = friendInfo->_back;
    seed.previewText = dialogItem.value(QStringLiteral("lastMsg"), friendInfo->_last_msg).toString();
    seed.sex = friendInfo->_sex;
    seed.pinnedRank = dialogItem.value(QStringLiteral("pinnedRank")).toInt();
    seed.draftText = dialogItem.value(QStringLiteral("draftText")).toString();
    seed.lastMsgTs = dialogItem.value(QStringLiteral("lastMsgTs")).toLongLong();
    seed.muteState = dialogItem.value(QStringLiteral("muteState")).toInt();
    if (seed.lastMsgTs <= 0 && !friendInfo->_chat_msgs.empty() && friendInfo->_chat_msgs.back())
    {
        seed.lastMsgTs = friendInfo->_chat_msgs.back()->_created_at;
    }
    DialogListService::applyFriendProfileToPrivateSeed(seed, friendInfo);
    const DialogDecorationState decorationState =
        port.dialogDecorationState ? port.dialogDecorationState() : DialogDecorationState{};
    auto dialogEntry = DialogListService::buildDialogEntry(seed, decorationState);
    if (dialogEntry)
    {
        if (port.chatListModel)
        {
            port.chatListModel->upsertFriend(dialogEntry);
        }
        if (port.dialogListModel)
        {
            port.dialogListModel->upsertFriend(dialogEntry);
        }
    }

    if (port.setCurrentPrivatePeerUid)
    {
        port.setCurrentPrivatePeerUid(uid);
    }
    if (port.dialogListModel)
    {
        port.dialogListModel->clearUnread(uid);
    }
    if (port.chatListModel)
    {
        port.chatListModel->clearUnread(uid);
    }
    if (port.setCurrentGroup)
    {
        port.setCurrentGroup(0, QString(), QString());
    }
    if (port.setCurrentChatPeerName)
    {
        port.setCurrentChatPeerName(DialogListService::privateDisplayName(friendInfo));
    }
    if (port.setCurrentChatPeerIcon)
    {
        port.setCurrentChatPeerIcon(normalizedPrivateIcon(friendInfo->_icon));
    }
    if (port.emitCurrentDialogUidChangedIfNeeded)
    {
        port.emitCurrentDialogUidChangedIfNeeded();
    }
    qInfo() << "Private chat resolved, uid:" << uid << "name:" << friendInfo->_name
            << "friend msg count:" << static_cast<qlonglong>(friendInfo->_chat_msgs.size());
    if (port.loadCurrentPrivateMessages)
    {
        port.loadCurrentPrivateMessages();
    }
    if (port.syncCurrentDialogDraft)
    {
        port.syncCurrentDialogDraft();
    }
}

void ChatDialogSelectionService::selectGroupByIndex(int index, const ChatDialogSelectionPort& port)
{
    if (port.ensureGroupsInitialized)
    {
        port.ensureGroupsInitialized();
    }
    const QVariantMap item = groupDialogByIndex(port, index);
    if (item.isEmpty())
    {
        clearCurrentProjection(port, false);
        return;
    }

    const int pseudoUid = item.value(QStringLiteral("uid")).toInt();
    const qint64 groupId = groupIdForDialogUid(port, pseudoUid);
    if (groupId <= 0)
    {
        clearCurrentProjection(port, false);
        return;
    }

    QString groupCode;
    const auto groupInfo = port.groupById ? port.groupById(groupId) : nullptr;
    if (groupInfo)
    {
        groupCode = groupInfo->_group_code;
    }
    if (port.setPendingReplyContext)
    {
        port.setPendingReplyContext(QString(), QString(), QString());
    }
    if (port.setCurrentGroup)
    {
        port.setCurrentGroup(groupId, item.value(QStringLiteral("name")).toString(), groupCode);
    }
    if (port.dialogListModel)
    {
        port.dialogListModel->clearUnread(pseudoUid);
        port.dialogListModel->clearMention(pseudoUid);
    }
    if (port.removeMentionForDialog)
    {
        port.removeMentionForDialog(pseudoUid);
    }
    if (port.setCurrentPrivatePeerUid)
    {
        port.setCurrentPrivatePeerUid(0);
    }
    if (port.resetGroupConversationState)
    {
        port.resetGroupConversationState();
    }
    if (port.setPrivateHistoryLoading)
    {
        port.setPrivateHistoryLoading(false);
    }
    if (port.setCanLoadMorePrivateHistory)
    {
        port.setCanLoadMorePrivateHistory(true);
    }
    if (port.setCurrentChatPeerName)
    {
        port.setCurrentChatPeerName(item.value(QStringLiteral("name")).toString());
    }
    if (port.setCurrentChatPeerIcon)
    {
        port.setCurrentChatPeerIcon(normalizedGroupIcon(item.value(QStringLiteral("icon")).toString()));
    }
    if (port.emitCurrentDialogUidChangedIfNeeded)
    {
        port.emitCurrentDialogUidChangedIfNeeded();
    }

    openCurrentGroupConversation(groupId, port);
}

void ChatDialogSelectionService::selectGroupByDialogUid(int dialogUid,
                                                        qint64 groupId,
                                                        const ChatDialogSelectionPort& port)
{
    if (dialogUid >= 0 || groupId <= 0)
    {
        return;
    }

    QVariantMap item;
    const int groupIndex = indexOfGroupDialog(port, dialogUid);
    if (groupIndex >= 0)
    {
        item = groupDialogByIndex(port, groupIndex);
    }
    if (item.isEmpty())
    {
        const int dialogIndex = port.dialogListModel ? port.dialogListModel->indexOfUid(dialogUid) : -1;
        if (dialogIndex >= 0 && port.dialogListModel)
        {
            item = port.dialogListModel->get(dialogIndex);
        }
    }

    auto groupInfo = port.groupById ? port.groupById(groupId) : nullptr;
    QString groupName = item.value(QStringLiteral("name")).toString();
    QString groupIcon = item.value(QStringLiteral("icon")).toString();
    QString groupCode;
    if (groupInfo)
    {
        if (groupName.trimmed().isEmpty())
        {
            groupName = groupInfo->_name;
        }
        if (groupIcon.trimmed().isEmpty())
        {
            groupIcon = groupInfo->_icon;
        }
        groupCode = groupInfo->_group_code;
    }
    if (groupName.trimmed().isEmpty())
    {
        groupName = QString("群聊%1").arg(groupId);
    }
    if (!groupInfo)
    {
        groupInfo = std::make_shared<GroupInfoData>();
        groupInfo->_group_id = groupId;
        groupInfo->_name = groupName;
        groupInfo->_icon = groupIcon;
        if (port.upsertGroup)
        {
            port.upsertGroup(groupInfo);
        }
    }

    if (port.setPendingReplyContext)
    {
        port.setPendingReplyContext(QString(), QString(), QString());
    }
    if (port.setCurrentGroup)
    {
        port.setCurrentGroup(groupId, groupName, groupCode);
    }
    if (port.dialogListModel)
    {
        port.dialogListModel->clearUnread(dialogUid);
        port.dialogListModel->clearMention(dialogUid);
    }
    clearGroupUnread(port, dialogUid);
    if (port.removeMentionForDialog)
    {
        port.removeMentionForDialog(dialogUid);
    }
    if (port.setCurrentPrivatePeerUid)
    {
        port.setCurrentPrivatePeerUid(0);
    }
    if (port.resetGroupConversationState)
    {
        port.resetGroupConversationState();
    }
    if (port.setGroupHistoryLoading)
    {
        port.setGroupHistoryLoading(false);
    }
    if (port.setPrivateHistoryLoading)
    {
        port.setPrivateHistoryLoading(false);
    }
    if (port.setCanLoadMorePrivateHistory)
    {
        port.setCanLoadMorePrivateHistory(true);
    }
    if (port.setCurrentChatPeerName)
    {
        port.setCurrentChatPeerName(groupName);
    }
    if (port.setCurrentChatPeerIcon)
    {
        port.setCurrentChatPeerIcon(normalizedGroupIcon(groupIcon));
    }
    if (port.emitCurrentDialogUidChangedIfNeeded)
    {
        port.emitCurrentDialogUidChangedIfNeeded();
    }

    openCurrentGroupConversation(groupId, port);
}

void ChatDialogSelectionService::selectDialogByUid(int dialogUid, const ChatDialogSelectionPort& port)
{
    if (dialogUid == 0)
    {
        return;
    }

    qInfo() << "Selecting dialog by uid:" << dialogUid;
    if (dialogUid > 0)
    {
        selectPrivateByUid(dialogUid, port);
        return;
    }

    const int groupIndex = indexOfGroupDialog(port, dialogUid);
    if (groupIndex >= 0)
    {
        selectGroupByIndex(groupIndex, port);
        return;
    }

    const qint64 groupId = groupIdForDialogUid(port, dialogUid);
    if (groupId > 0)
    {
        selectGroupByDialogUid(dialogUid, groupId, port);
    }
}
