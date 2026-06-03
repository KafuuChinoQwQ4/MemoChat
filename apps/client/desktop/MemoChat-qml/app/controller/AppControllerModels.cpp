#include "AppController.h"
#include "ConversationSyncService.h"
#include "usermgr.h"

void AppController::refreshFriendModels()
{
    ensureChatListInitialized();
    bootstrapContacts();
    bootstrapGroups();
    refreshDialogModel();
}

void AppController::refreshApplyModel()
{
    const auto applyList = _gateway.userMgr()->GetApplyListSnapshot();
    _apply_request_model.setApplies(applyList);
}

void AppController::bootstrapDialogs()
{
    if (_bootstrap_state.dialogsReady)
    {
        return;
    }
    _bootstrap_state.dialogBootstrapLoading = true;
    setDialogsReady(false);
    requestDialogList();
}

void AppController::bootstrapContacts()
{
    if (_bootstrap_state.contactsReady)
    {
        return;
    }

    ensureChatListInitialized();
    _contact_list_model.clear();
    const auto contactList = _gateway.userMgr()->GetConListPerPage();
    _contact_list_model.setFriends(contactList);
    _gateway.userMgr()->UpdateContactLoadedCount();
    refreshContactLoadMoreState();
    setContactsReady(true);
}

void AppController::bootstrapGroups()
{
    if (_bootstrap_state.groupsReady)
    {
        return;
    }

    refreshGroupModel();
    setGroupsReady(true);
    refreshGroupList();
}

void AppController::bootstrapApplies()
{
    if (_bootstrap_state.applyReady)
    {
        return;
    }

    refreshApplyModel();
    setApplyReady(true);
}

void AppController::refreshGroupModel()
{
    _group_list_model.clear();
    _group_state.dialogUidMap.clear();

    const auto groups = _gateway.userMgr()->GetGroupListSnapshot();
    std::vector<std::shared_ptr<FriendInfo>> converted;
    converted.reserve(groups.size());
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
        const QString groupIcon =
            group->_icon.trimmed().isEmpty() ? QStringLiteral("qrc:/res/chat_icon.png") : group->_icon;
        auto info = std::make_shared<FriendInfo>(pseudoUid,
                                                 group->_name,
                                                 group->_name,
                                                 groupIcon,
                                                 0,
                                                 group->_announcement,
                                                 group->_announcement,
                                                 group->_last_msg);
        converted.push_back(info);
    }

    _group_list_model.setFriends(converted);
    syncGroupControllerState();
}
