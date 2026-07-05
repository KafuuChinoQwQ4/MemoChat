#include "AppController.h"
#include "usermgr.h"

void AppController::refreshFriendModels()
{
    ensureChatListInitialized();
    bootstrapContacts();
    bootstrapGroups();
    refreshDialogModel();
}

void AppController::bootstrapDialogs()
{
    if (_shell_state.bootstrapState().dialogsReady)
    {
        return;
    }
    if (!isChatTransportReady())
    {
        // Transport not connected yet; bootstrap will be retried when the
        // chat login succeeds on the next connect / reconnect attempt.
        return;
    }
    _shell_state.bootstrapState().dialogBootstrapLoading = true;
    setDialogsReady(false);
    requestDialogList();
}

void AppController::bootstrapContacts()
{
    _features.contactController.ensureContactsInitialized();
}

void AppController::bootstrapGroups()
{
    if (_shell_state.bootstrapState().groupsReady)
    {
        return;
    }
    if (!isChatTransportReady())
    {
        // Transport not connected yet; will be retried via onSwitchToChat() on reconnect.
        return;
    }

    refreshGroupModel();
    setGroupsReady(true);
    refreshGroupList();
}

void AppController::refreshGroupModel()
{
    _features.chatFeatureController.resetGroupDialogIdentityMap();
    _features.groupController.setGroupsFromSnapshots(_gateway.userMgr()->GetGroupListSnapshot(),
                                                     _features.chatFeatureController.groupDialogUidMap());
}
