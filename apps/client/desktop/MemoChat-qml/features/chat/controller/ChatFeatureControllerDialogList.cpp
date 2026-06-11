#include "ChatFeatureController.h"

#include "ConversationSyncService.h"
#include "DialogListService.h"
#include "FriendListModel.h"

#include <utility>

namespace
{
ChatDialogSnapshotRefreshResult refreshResultFor(const std::vector<std::shared_ptr<FriendInfo>>& dialogs)
{
    ChatDialogSnapshotRefreshResult result;
    result.totalCount = static_cast<int>(dialogs.size());
    for (const auto& dialog : dialogs)
    {
        if (!dialog)
        {
            continue;
        }
        if (dialog->_uid > 0)
        {
            ++result.privateCount;
        }
        else if (dialog->_uid < 0)
        {
            ++result.groupCount;
        }
    }
    return result;
}
} // namespace

QMap<int, qint64>& ChatFeatureController::groupDialogUidMap()
{
    return _groupDialogUidMap;
}

const QMap<int, qint64>& ChatFeatureController::groupDialogUidMap() const
{
    return _groupDialogUidMap;
}

QMap<int, qint64>* ChatFeatureController::groupDialogUidMapPtr()
{
    return &_groupDialogUidMap;
}

void ChatFeatureController::resetGroupDialogIdentityMap()
{
    _groupDialogUidMap.clear();
}

int ChatFeatureController::resolveGroupDialogUid(qint64 groupId)
{
    return ConversationSyncService::resolveGroupDialogUid(_groupDialogUidMap, groupId);
}

qint64 ChatFeatureController::groupIdForDialogUid(int dialogUid) const
{
    return ConversationSyncService::groupIdForDialogUid(_groupDialogUidMap, dialogUid);
}

void ChatFeatureController::removeGroupDialogUid(int dialogUid)
{
    _groupDialogUidMap.remove(dialogUid);
}

ChatDialogSnapshotRefreshResult
ChatFeatureController::replaceDialogListFromSnapshots(const std::vector<std::shared_ptr<FriendInfo>>& friends,
                                                      const std::vector<std::shared_ptr<GroupInfoData>>& groups)
{
    const std::vector<std::shared_ptr<FriendInfo>> dialogs =
        DialogListService::buildSortedSnapshotDialogs(friends, groups, _groupDialogUidMap, dialogDecorationState());
    _dialogListModel.clear();
    _dialogListModel.upsertBatch(dialogs, true);
    return refreshResultFor(dialogs);
}

ChatDialogSnapshotRefreshResult
ChatFeatureController::upsertDialogListFromSnapshots(const std::vector<std::shared_ptr<FriendInfo>>& friends,
                                                     const std::vector<std::shared_ptr<GroupInfoData>>& groups)
{
    const std::vector<std::shared_ptr<FriendInfo>> dialogs =
        DialogListService::buildSortedSnapshotDialogs(friends, groups, _groupDialogUidMap, dialogDecorationState());
    for (const auto& dialog : dialogs)
    {
        if (dialog && dialog->_uid > 0)
        {
            _chatListModel.upsertFriend(dialog);
        }
    }
    _dialogListModel.upsertBatch(dialogs);
    return refreshResultFor(dialogs);
}

ChatDialogListResponseEffect ChatFeatureController::handleDialogListResponse(const QJsonObject& payload,
                                                                             ChatDialogListResponseContext context,
                                                                             const ChatDialogListAppPort& appPort)
{
    context.decorationState = dialogDecorationStateWithoutServerMeta();
    context.existingDialogs.reserve(static_cast<size_t>(_dialogListModel.count()));
    for (int i = 0; i < _dialogListModel.count(); ++i)
    {
        context.existingDialogs.push_back(_dialogListModel.get(i));
    }

    ChatDialogListResponsePort featurePort;
    featurePort.resetServerDialogMeta = [this]()
    {
        resetServerDialogMeta();
    };
    featurePort.seedServerDialogMeta = [this](int dialogUid, int pinnedRank, int muteState)
    {
        seedServerDialogMeta(dialogUid, pinnedRank, muteState);
    };
    featurePort.upsertChatDialog = [this](std::shared_ptr<FriendInfo> dialog)
    {
        _chatListModel.upsertFriend(std::move(dialog));
    };
    featurePort.replaceDialogList = [this](const std::vector<std::shared_ptr<FriendInfo>>& dialogs)
    {
        _dialogListModel.upsertBatch(dialogs, true);
    };
    featurePort.clearUnreadDialog = [this](int dialogUid)
    {
        _dialogListModel.clearUnread(dialogUid);
    };
    featurePort.clearMentionDialog = [this](int dialogUid)
    {
        _dialogListModel.clearMention(dialogUid);
    };
    featurePort.removeMentionForDialog = [this](int dialogUid)
    {
        removeMentionForDialog(dialogUid);
    };
    ChatDialogListAppPort guardedAppPort = appPort;
    guardedAppPort.selectFirstChat = [this, selectFirstChat = appPort.selectFirstChat]()
    {
        if (_chatListModel.count() <= 0)
        {
            return;
        }
        if (selectFirstChat)
        {
            selectFirstChat();
        }
    };

    return ChatDialogListResponseService::handle(payload,
                                                 std::move(context),
                                                 _groupDialogUidMap,
                                                 featurePort,
                                                 guardedAppPort);
}
