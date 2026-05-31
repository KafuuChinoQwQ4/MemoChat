#include "AppController.h"
#include "AppChatConnectionCoordinator.h"
#include "IconPathUtils.h"
#include "IChatTransport.h"
#include "httpmgr.h"
#include "usermgr.h"

#include <QDebug>
#include <QVariantList>

void AppController::switchToLogin()
{
    qInfo() << "Switching to login page, current page:" << _page << "pending uid:" << _pending_login_state.uid
            << "chat connected:" << _gateway.chatTransport()->isConnected();
    const bool already_on_login_page = _page == LoginPage;
    _register_countdown_timer.stop();
    _heartbeat_timer.stop();
    _chat_login_timeout_timer.stop();
    _chat_recovery_state.ignoreNextLoginDisconnect = true;
    _bootstrap_state.postLoginBootstrapStarted = false;
    setPage(LoginPage);
    if (already_on_login_page)
    {
        emit pageChanged();
    }
    _livekit_bridge.leaveRoom();
    _pet_controller.stopStream();
    _call_session_model.clear();
    _chat_endpoint_state.host.clear();
    _chat_endpoint_state.port.clear();
    _chat_endpoint_state.serverName.clear();
    _chat_endpoint_state.endpoints.clear();
    _chat_endpoint_state.endpointIndex = -1;
    _pending_login_state.loginTicket.clear();
    _chat_recovery_state.loginTcpFallbackAttempted = false;
    _chat_connection_coordinator->resetReconnectState();
    _chat_connection_coordinator->resetHeartbeatTracking();
    _gateway.chatTransport()->CloseConnection();
    if (auto http = _gateway.httpMgr())
    {
        http->clearConnectionCache();
    }
    _private_cache_store.close();
    _group_cache_store.close();
    _gateway.userMgr()->ResetSession();
    if (_shell_state.registerSuccessPage)
    {
        _shell_state.registerSuccessPage = false;
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
    _private_history_state.beforeTs = 0;
    _private_history_state.beforeMsgId.clear();
    _private_history_state.pendingBeforeTs = 0;
    _private_history_state.pendingBeforeMsgId.clear();
    _private_history_state.pendingPeerUid = 0;
    _group_state.historyBeforeSeq = 0;
    _group_state.historyHasMore = true;
    _loading_state.groupHistoryLoading = false;
    _bootstrap_state.dialogBootstrapLoading = false;
    _bootstrap_state.chatListInitialized = false;
    setDialogsReady(false);
    setContactsReady(false);
    setGroupsReady(false);
    setApplyReady(false);
    _loading_state.canLoadMorePrivateHistory = false;
    emit canLoadMorePrivateHistoryChanged();
    setContactLoadingMore(false);
    setAuthStatus(QString(), false);
    setSettingsStatus(QString(), false);
    setGroupStatus(QString(), false);
    setContactPane(ApplyRequestPane);
    _loading_state.canLoadMoreChats = false;
    emit canLoadMoreChatsChanged();
    _loading_state.canLoadMoreContacts = false;
    emit canLoadMoreContactsChanged();
    setCurrentContact(0, QString(), QString(), QStringLiteral("qrc:/res/head_1.png"), QString(), 0);
    _chat_state.uid = 0;
    _group_state.currentId = 0;
    _group_state.currentName.clear();
    _group_state.currentCode.clear();
    emit currentGroupChanged();
    emitCurrentDialogUidChangedIfNeeded();
    _group_state.dialogUidMap.clear();
    _group_state.pendingMsgGroupMap.clear();
    _dialog_state.draftMap.clear();
    _dialog_state.pendingAttachmentMap.clear();
    _dialog_state.localPinnedSet.clear();
    _dialog_state.serverPinnedMap.clear();
    _dialog_state.serverMuteMap.clear();
    _dialog_state.mentionMap.clear();
    setCurrentDraftText(QString());
    setCurrentPendingAttachments(QVariantList());
    setCurrentDialogPinned(false);
    setCurrentDialogMuted(false);
    setPendingReplyContext(QString(), QString(), QString());
    setCurrentChatPeerName(QString());
    setCurrentChatPeerIcon(QStringLiteral("qrc:/res/head_1.png"));
    const bool userChanged = !_user_state.name.isEmpty() || !_user_state.nick.isEmpty() ||
                             _user_state.icon != QStringLiteral("qrc:/res/head_1.png") ||
                                                                !_user_state.userId.isEmpty() ||
                                                                !_user_state.desc.isEmpty();
    _user_state.name.clear();
    _user_state.nick.clear();
    _user_state.icon = QStringLiteral("qrc:/res/head_1.png");
    _user_state.userId.clear();
    _user_state.desc.clear();
    if (userChanged)
    {
        emit currentUserChanged();
    }
    _pending_login_state.uid = 0;
    _pending_login_state.token.clear();
    _pending_login_state.traceId.clear();
    _pending_send_state.reset();
    setMediaUploadInProgress(false);
    setMediaUploadProgressText(QString());
    _message_model.setDownloadAuthContext(0, QString());
    setIconDownloadAuthContext(0, QString());
    _media_upload_state.settingsAvatarUploadInProgress = false;
    _media_upload_state.groupIconUploadInProgress = false;
}

void AppController::switchToRegister()
{
    _register_countdown_timer.stop();
    _heartbeat_timer.stop();
    _chat_connection_coordinator->resetHeartbeatTracking();
    if (_shell_state.registerSuccessPage)
    {
        _shell_state.registerSuccessPage = false;
        emit registerSuccessPageChanged();
    }
    setPage(RegisterPage);
    setTip(QString(), false);
}

void AppController::switchToReset()
{
    _register_countdown_timer.stop();
    _heartbeat_timer.stop();
    _chat_connection_coordinator->resetHeartbeatTracking();
    if (_shell_state.registerSuccessPage)
    {
        _shell_state.registerSuccessPage = false;
        emit registerSuccessPageChanged();
    }
    setPage(ResetPage);
    setTip(QString(), false);
}

void AppController::switchChatTab(int tab)
{
    const int normalized = qBound(0, tab, static_cast<int>(Live2DTabPage));
    const ChatTab target = static_cast<ChatTab>(normalized);
    if (_chat_tab == target)
    {
        return;
    }
    _chat_tab = target;
    if (target == ContactTabPage)
    {
        ensureContactsInitialized();
        ensureApplyInitialized();
    }
    else if (target == MomentsTabPage)
    {
        if (_moments_controller.model()->count() == 0)
        {
            _moments_controller.loadFeed();
        }
    }
    else if (target == AgentTabPage)
    {
        _agent_controller.loadSessions();
        _agent_controller.refreshModelList();
    }
    emit chatTabChanged();
}

void AppController::ensureContactsInitialized()
{
    if (_bootstrap_state.contactsReady)
    {
        return;
    }
    bootstrapContacts();
}

void AppController::ensureGroupsInitialized()
{
    if (_bootstrap_state.groupsReady)
    {
        return;
    }
    bootstrapGroups();
}

void AppController::ensureApplyInitialized()
{
    if (_bootstrap_state.applyReady)
    {
        return;
    }
    bootstrapApplies();
}

void AppController::ensureChatListInitialized()
{
    if (_bootstrap_state.chatListInitialized)
    {
        return;
    }

    _chat_list_model.clear();
    const auto chatList = _gateway.userMgr()->GetChatListPerPage();
    _chat_list_model.setFriends(chatList);
    _gateway.userMgr()->UpdateChatLoadedCount();
    _bootstrap_state.chatListInitialized = true;
    refreshChatLoadMoreState();
}
