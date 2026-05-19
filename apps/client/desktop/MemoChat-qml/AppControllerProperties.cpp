#include "AppController.h"
#include "usermgr.h"

namespace {
constexpr qint64 kPermChangeGroupInfo = 1LL << 0;
constexpr qint64 kPermDeleteMessages = 1LL << 1;
constexpr qint64 kPermInviteUsers = 1LL << 2;
constexpr qint64 kPermManageAdmins = 1LL << 3;
constexpr qint64 kPermPinMessages = 1LL << 4;
constexpr qint64 kPermBanUsers = 1LL << 5;
constexpr qint64 kPermManageTopics = 1LL << 6;
}

QString AppController::tipText() const
{
    return _tip_text;
}

bool AppController::tipError() const
{
    return _tip_error;
}

bool AppController::busy() const
{
    return _busy;
}

bool AppController::registerSuccessPage() const
{
    return _register_success_page;
}

int AppController::registerCountdown() const
{
    return _register_countdown;
}

AppController::ChatTab AppController::chatTab() const
{
    return _chat_tab;
}

AppController::ContactPane AppController::contactPane() const
{
    return _contact_pane;
}

QString AppController::currentUserName() const
{
    return _current_user_name;
}

QString AppController::currentUserNick() const
{
    return _current_user_nick;
}

QString AppController::currentUserIcon() const
{
    return _current_user_icon;
}

QString AppController::currentUserDesc() const
{
    return _current_user_desc;
}

QString AppController::currentUserId() const
{
    return _current_user_id;
}

int AppController::currentUserUid() const
{
    const auto userMgr = _gateway.userMgr();
    return userMgr ? userMgr->GetUid() : 0;
}

QString AppController::currentContactName() const
{
    return _current_contact_name;
}

QString AppController::currentContactNick() const
{
    return _current_contact_nick;
}

QString AppController::currentContactIcon() const
{
    return _current_contact_icon;
}

QString AppController::currentContactBack() const
{
    return _current_contact_back;
}

int AppController::currentContactSex() const
{
    return _current_contact_sex;
}

QString AppController::currentContactUserId() const
{
    return _current_contact_user_id;
}

int AppController::currentContactUid() const
{
    return _current_contact_uid;
}

bool AppController::hasCurrentContact() const
{
    return _current_contact_uid > 0;
}

QString AppController::currentChatPeerName() const
{
    return _current_chat_peer_name;
}

QString AppController::currentChatPeerIcon() const
{
    return _current_chat_peer_icon;
}

bool AppController::hasCurrentChat() const
{
    return _current_chat_uid > 0 || _current_group_id > 0;
}

bool AppController::hasCurrentGroup() const
{
    return _current_group_id > 0;
}

int AppController::currentGroupRole() const
{
    if (_current_group_id <= 0) {
        return 0;
    }
    auto groupInfo = _gateway.userMgr()->GetGroupById(_current_group_id);
    if (!groupInfo) {
        return 0;
    }
    return groupInfo->_role;
}

QString AppController::currentGroupName() const
{
    return _current_group_name;
}

QString AppController::currentGroupCode() const
{
    return _current_group_code;
}

bool AppController::currentGroupCanChangeInfo() const
{
    return hasCurrentGroupPermission(kPermChangeGroupInfo);
}

bool AppController::currentGroupCanDeleteMessages() const
{
    return hasCurrentGroupPermission(kPermDeleteMessages);
}

bool AppController::currentGroupCanInviteUsers() const
{
    return hasCurrentGroupPermission(kPermInviteUsers);
}

bool AppController::currentGroupCanManageAdmins() const
{
    return hasCurrentGroupPermission(kPermManageAdmins);
}

bool AppController::currentGroupCanPinMessages() const
{
    return hasCurrentGroupPermission(kPermPinMessages);
}

bool AppController::currentGroupCanBanUsers() const
{
    return hasCurrentGroupPermission(kPermBanUsers);
}

bool AppController::currentGroupCanManageTopics() const
{
    return hasCurrentGroupPermission(kPermManageTopics);
}

FriendListModel *AppController::dialogListModel()
{
    return &_dialog_list_model;
}

FriendListModel *AppController::chatListModel()
{
    return &_chat_list_model;
}

FriendListModel *AppController::groupListModel()
{
    return &_group_list_model;
}

FriendListModel *AppController::contactListModel()
{
    return &_contact_list_model;
}

ChatMessageModel *AppController::messageModel()
{
    return &_message_model;
}

SearchResultModel *AppController::searchResultModel()
{
    return &_search_result_model;
}

ApplyRequestModel *AppController::applyRequestModel()
{
    return &_apply_request_model;
}

MomentsModel *AppController::momentsModel() const
{
    return _moments_controller.model();
}

MomentsController *AppController::momentsController() const
{
    return const_cast<MomentsController*>(&_moments_controller);
}

AgentController *AppController::agentController() const
{
    return const_cast<AgentController*>(&_agent_controller);
}

AgentMessageModel *AppController::agentMessageModel() const
{
    return _agent_controller.model();
}

PetController *AppController::petController() const
{
    return const_cast<PetController*>(&_pet_controller);
}

R18Controller *AppController::r18Controller() const
{
    return const_cast<R18Controller*>(&_r18_controller);
}

bool AppController::searchPending() const
{
    return _search_pending;
}

QString AppController::searchStatusText() const
{
    return _search_status_text;
}

bool AppController::searchStatusError() const
{
    return _search_status_error;
}

bool AppController::hasPendingApply() const
{
    return _apply_request_model.hasUnapproved();
}

bool AppController::chatLoadingMore() const
{
    return _chat_loading_more;
}

bool AppController::canLoadMoreChats() const
{
    return _can_load_more_chats;
}

bool AppController::privateHistoryLoading() const
{
    return _private_history_loading;
}

bool AppController::canLoadMorePrivateHistory() const
{
    return _can_load_more_private_history;
}

bool AppController::contactLoadingMore() const
{
    return _contact_loading_more;
}

bool AppController::canLoadMoreContacts() const
{
    return _can_load_more_contacts;
}

QString AppController::authStatusText() const
{
    return _auth_status_text;
}

bool AppController::authStatusError() const
{
    return _auth_status_error;
}

QString AppController::settingsStatusText() const
{
    return _settings_status_text;
}

bool AppController::settingsStatusError() const
{
    return _settings_status_error;
}

QString AppController::groupStatusText() const
{
    return _group_status_text;
}

bool AppController::groupStatusError() const
{
    return _group_status_error;
}

bool AppController::mediaUploadInProgress() const
{
    return _media_upload_in_progress;
}

QString AppController::mediaUploadProgressText() const
{
    return _media_upload_progress_text;
}

QString AppController::currentDraftText() const
{
    return _current_draft_text;
}

QVariantList AppController::currentPendingAttachments() const
{
    return _current_pending_attachments;
}

bool AppController::hasPendingAttachments() const
{
    return !_current_pending_attachments.isEmpty();
}

bool AppController::currentDialogPinned() const
{
    return _current_dialog_pinned;
}

bool AppController::currentDialogMuted() const
{
    return _current_dialog_muted;
}

bool AppController::hasPendingReply() const
{
    return !_reply_to_msg_id.isEmpty();
}

QString AppController::replyTargetName() const
{
    return _reply_target_name;
}

QString AppController::replyPreviewText() const
{
    return _reply_preview_text;
}

bool AppController::dialogsReady() const
{
    return _dialogs_ready;
}

bool AppController::contactsReady() const
{
    return _contacts_ready;
}

bool AppController::groupsReady() const
{
    return _groups_ready;
}

bool AppController::applyReady() const
{
    return _apply_ready;
}

bool AppController::chatShellBusy() const
{
    return _page == ChatPage && !_dialogs_ready;
}
