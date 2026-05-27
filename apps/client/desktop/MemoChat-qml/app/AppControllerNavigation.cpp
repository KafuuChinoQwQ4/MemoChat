#include "AppController.h"
#include "IconPathUtils.h"
#include "IChatTransport.h"
#include "httpmgr.h"
#include "usermgr.h"

#include <QDebug>
#include <QVariantList>

void AppController::switchToLogin()
{
    qInfo() << "Switching to login page, current page:" << _page
            << "pending uid:" << _pending_uid
            << "chat connected:" << _gateway.chatTransport()->isConnected();
    const bool already_on_login_page = _page == LoginPage;
    _register_countdown_timer.stop();
    _heartbeat_timer.stop();
    _chat_login_timeout_timer.stop();
    _ignore_next_login_disconnect = true;
    _post_login_bootstrap_started = false;
    setPage(LoginPage);
    if (already_on_login_page) {
        emit pageChanged();
    }
    _livekit_bridge.leaveRoom();
    _pet_controller.stopStream();
    _call_session_model.clear();
    _chat_server_host.clear();
    _chat_server_port.clear();
    _chat_server_name.clear();
    _chat_endpoints.clear();
    _chat_endpoint_index = -1;
    _pending_login_ticket.clear();
    _chat_login_tcp_fallback_attempted = false;
    resetReconnectState();
    resetHeartbeatTracking();
    _gateway.chatTransport()->CloseConnection();
    if (auto http = _gateway.httpMgr()) {
        http->clearConnectionCache();
    }
    _private_cache_store.close();
    _group_cache_store.close();
    _gateway.userMgr()->ResetSession();
    if (_register_success_page) {
        _register_success_page = false;
        emit registerSuccessPageChanged();
    }
    setBusy(false);
    setTip(QString(), false);
    _chat_list_model.clear();
    _group_list_model.clear();
    _dialog_list_model.clear();
    _contact_list_model.clear();
    _message_model.clear();
    _search_result_model.clear();
    _apply_request_model.clear();
    setSearchPending(false);
    setSearchStatus(QString(), false);
    setChatLoadingMore(false);
    setPrivateHistoryLoading(false);
    _private_history_before_ts = 0;
    _private_history_before_msg_id.clear();
    _private_history_pending_before_ts = 0;
    _private_history_pending_before_msg_id.clear();
    _private_history_pending_peer_uid = 0;
    _group_history_before_seq = 0;
    _group_history_has_more = true;
    _group_history_loading = false;
    _dialog_bootstrap_loading = false;
    _chat_list_initialized = false;
    setDialogsReady(false);
    setContactsReady(false);
    setGroupsReady(false);
    setApplyReady(false);
    _can_load_more_private_history = false;
    emit canLoadMorePrivateHistoryChanged();
    setContactLoadingMore(false);
    setAuthStatus(QString(), false);
    setSettingsStatus(QString(), false);
    setGroupStatus(QString(), false);
    setContactPane(ApplyRequestPane);
    _can_load_more_chats = false;
    emit canLoadMoreChatsChanged();
    _can_load_more_contacts = false;
    emit canLoadMoreContactsChanged();
    setCurrentContact(0, QString(), QString(), QStringLiteral("qrc:/res/head_1.png"), QString(), 0);
    _current_chat_uid = 0;
    _current_group_id = 0;
    _current_group_name.clear();
    _current_group_code.clear();
    emit currentGroupChanged();
    emitCurrentDialogUidChangedIfNeeded();
    _group_uid_map.clear();
    _pending_group_msg_group_map.clear();
    _dialog_draft_map.clear();
    _dialog_pending_attachment_map.clear();
    _dialog_local_pinned_set.clear();
    _dialog_server_pinned_map.clear();
    _dialog_server_mute_map.clear();
    _dialog_mention_map.clear();
    setCurrentDraftText(QString());
    setCurrentPendingAttachments(QVariantList());
    setCurrentDialogPinned(false);
    setCurrentDialogMuted(false);
    setPendingReplyContext(QString(), QString(), QString());
    setCurrentChatPeerName(QString());
    setCurrentChatPeerIcon(QStringLiteral("qrc:/res/head_1.png"));
    const bool userChanged = !_current_user_name.isEmpty()
        || !_current_user_nick.isEmpty()
        || _current_user_icon != QStringLiteral("qrc:/res/head_1.png")
        || !_current_user_id.isEmpty()
        || !_current_user_desc.isEmpty();
    _current_user_name.clear();
    _current_user_nick.clear();
    _current_user_icon = QStringLiteral("qrc:/res/head_1.png");
    _current_user_id.clear();
    _current_user_desc.clear();
    if (userChanged) {
        emit currentUserChanged();
    }
    _pending_uid = 0;
    _pending_token.clear();
    _pending_trace_id.clear();
    _pending_send_queue.clear();
    _pending_send_total_count = 0;
    _pending_send_dialog_uid = 0;
    _pending_send_chat_uid = 0;
    _pending_send_group_id = 0;
    setMediaUploadInProgress(false);
    setMediaUploadProgressText(QString());
    _message_model.setDownloadAuthContext(0, QString());
    setIconDownloadAuthContext(0, QString());
    _settings_avatar_upload_in_progress = false;
    _group_icon_upload_in_progress = false;
}

void AppController::switchToRegister()
{
    _register_countdown_timer.stop();
    _heartbeat_timer.stop();
    resetHeartbeatTracking();
    if (_register_success_page) {
        _register_success_page = false;
        emit registerSuccessPageChanged();
    }
    setPage(RegisterPage);
    setTip(QString(), false);
}

void AppController::switchToReset()
{
    _register_countdown_timer.stop();
    _heartbeat_timer.stop();
    resetHeartbeatTracking();
    if (_register_success_page) {
        _register_success_page = false;
        emit registerSuccessPageChanged();
    }
    setPage(ResetPage);
    setTip(QString(), false);
}

void AppController::switchChatTab(int tab)
{
    const int normalized = qBound(0, tab, static_cast<int>(Live2DTabPage));
    const ChatTab target = static_cast<ChatTab>(normalized);
    if (_chat_tab == target) {
        return;
    }
    _chat_tab = target;
    if (target == ContactTabPage) {
        ensureContactsInitialized();
        ensureApplyInitialized();
    } else if (target == MomentsTabPage) {
        if (_moments_controller.model()->count() == 0) {
            _moments_controller.loadFeed();
        }
    } else if (target == AgentTabPage) {
        _agent_controller.loadSessions();
        _agent_controller.refreshModelList();
    }
    emit chatTabChanged();
}

void AppController::ensureContactsInitialized()
{
    if (_contacts_ready) {
        return;
    }
    bootstrapContacts();
}

void AppController::ensureGroupsInitialized()
{
    if (_groups_ready) {
        return;
    }
    bootstrapGroups();
}

void AppController::ensureApplyInitialized()
{
    if (_apply_ready) {
        return;
    }
    bootstrapApplies();
}

void AppController::ensureChatListInitialized()
{
    if (_chat_list_initialized) {
        return;
    }

    _chat_list_model.clear();
    const auto chatList = _gateway.userMgr()->GetChatListPerPage();
    _chat_list_model.setFriends(chatList);
    _gateway.userMgr()->UpdateChatLoadedCount();
    _chat_list_initialized = true;
    refreshChatLoadMoreState();
}
