#include "AppController.h"
#include "AppChatConnectionCoordinator.h"
#include "AppCoordinators.h"
#include "ChatMessageDispatcher.h"
#include "LocalFilePickerService.h"
#include "IChatTransport.h"
#include "httpmgr.h"
#include <QDebug>
#include <utility>

AppController::AppController(QObject* parent)
    : QObject(parent)
    , _page(LoginPage)
    , _chat_tab(ChatTabPage)
    , _contact_pane(ApplyRequestPane)
    , _chat_list_model(this)
    , _group_list_model(this)
    , _dialog_list_model(this)
    , _contact_list_model(this)
    , _message_model(this)
    , _search_result_model(this)
    , _apply_request_model(this)
    , _shell_view_model(this)
    , _auth_controller(&_gateway)
    , _auth_service(&_gateway)
    , _auth_view_model(&_auth_service, this)
    , _call_controller(&_gateway, this)
    , _call_session_model(this)
    , _livekit_bridge(this)
    , _chat_controller(&_gateway)
    , _chat_view_model(this)
    , _contact_controller(&_gateway)
    , _group_controller(this)
    , _profile_controller(&_gateway, this)
    , _agent_controller(&_gateway)
    , _pet_controller(&_gateway)
    , _r18_controller(&_gateway)
    , _session_coordinator(std::make_unique<AppSessionCoordinator>(*this))
    , _contact_coordinator_shell(std::make_unique<ContactCoordinatorShell>(*this))
    , _group_coordinator(std::make_unique<GroupCoordinator>(*this))
    , _media_coordinator(std::make_unique<MediaCoordinator>(*this))
    , _call_coordinator(std::make_unique<CallCoordinator>(*this))
    , _profile_coordinator(std::make_unique<ProfileCoordinator>(*this))
    , _chat_connection_coordinator(std::make_unique<AppChatConnectionCoordinator>(*this))
{
    const auto chatTransport = _gateway.chatTransport();
    const auto chatDispatcher = _gateway.chatMessageDispatcher();

    connect(_gateway.httpMgr().get(), &HttpMgr::sig_login_mod_finish, this, &AppController::onLoginHttpFinished);
    connect(_gateway.httpMgr().get(), &HttpMgr::sig_reg_mod_finish, this, &AppController::onRegisterHttpFinished);
    connect(_gateway.httpMgr().get(), &HttpMgr::sig_reset_mod_finish, this, &AppController::onResetHttpFinished);
    connect(_gateway.httpMgr().get(), &HttpMgr::sig_settings_mod_finish, this, &AppController::onSettingsHttpFinished);
    connect(_gateway.httpMgr().get(),
            &HttpMgr::sig_call_mod_finish,
            this,
            [this](ReqId id, QString res, ErrorCodes err)
            {
                _call_coordinator->onCallHttpFinished(id, std::move(res), err);
            });

    connect(chatTransport.get(),
            &IChatTransport::sig_con_success,
            this,
            [this](bool success)
            {
                _chat_connection_coordinator->onTcpConnectFinished(success);
            });
    connect(chatDispatcher.get(),
            &ChatMessageDispatcher::sig_login_failed,
            this,
            [this](int err)
            {
                _chat_connection_coordinator->onChatLoginFailed(err);
            });
    connect(chatDispatcher.get(), &ChatMessageDispatcher::sig_swich_chatdlg, this, &AppController::onSwitchToChat);
    connect(chatDispatcher.get(), &ChatMessageDispatcher::sig_add_auth_friend, this, &AppController::onAddAuthFriend);
    connect(chatDispatcher.get(), &ChatMessageDispatcher::sig_auth_rsp, this, &AppController::onAuthRsp);
    connect(chatDispatcher.get(),
            &ChatMessageDispatcher::sig_delete_friend_rsp,
            this,
            &AppController::onDeleteFriendRsp);
    connect(chatDispatcher.get(), &ChatMessageDispatcher::sig_text_chat_msg, this, &AppController::onTextChatMsg);
    connect(chatDispatcher.get(), &ChatMessageDispatcher::sig_user_search, this, &AppController::onUserSearch);
    connect(chatDispatcher.get(), &ChatMessageDispatcher::sig_friend_apply, this, &AppController::onFriendApply);
    connect(chatDispatcher.get(),
            &ChatMessageDispatcher::sig_notify_offline,
            this,
            [this]()
            {
                _chat_connection_coordinator->onNotifyOffline();
            });
    connect(chatTransport.get(),
            &IChatTransport::sig_connection_closed,
            this,
            [this]()
            {
                _chat_connection_coordinator->onConnectionClosed();
            });
    connect(chatDispatcher.get(),
            &ChatMessageDispatcher::sig_group_list_updated,
            this,
            &AppController::onGroupListUpdated);
    connect(chatDispatcher.get(), &ChatMessageDispatcher::sig_group_invite, this, &AppController::onGroupInvite);
    connect(chatDispatcher.get(), &ChatMessageDispatcher::sig_group_apply, this, &AppController::onGroupApply);
    connect(chatDispatcher.get(),
            &ChatMessageDispatcher::sig_group_member_changed,
            this,
            &AppController::onGroupMemberChanged);
    connect(chatDispatcher.get(), &ChatMessageDispatcher::sig_group_chat_msg, this, &AppController::onGroupChatMsg);
    connect(chatDispatcher.get(), &ChatMessageDispatcher::sig_group_rsp, this, &AppController::onGroupRsp);
    connect(chatDispatcher.get(),
            &ChatMessageDispatcher::sig_relation_bootstrap_updated,
            this,
            &AppController::onRelationBootstrapUpdated);
    connect(chatDispatcher.get(), &ChatMessageDispatcher::sig_dialog_list_rsp, this, &AppController::onDialogListRsp);
    connect(chatDispatcher.get(),
            &ChatMessageDispatcher::sig_private_history_rsp,
            this,
            &AppController::onPrivateHistoryRsp);
    connect(chatDispatcher.get(),
            &ChatMessageDispatcher::sig_private_msg_changed,
            this,
            &AppController::onPrivateMsgChanged);
    connect(chatDispatcher.get(), &ChatMessageDispatcher::sig_private_read_ack, this, &AppController::onPrivateReadAck);
    connect(chatDispatcher.get(), &ChatMessageDispatcher::sig_message_status, this, &AppController::onMessageStatus);
    connect(chatDispatcher.get(),
            &ChatMessageDispatcher::sig_call_event,
            this,
            [this](QJsonObject payload)
            {
                _call_coordinator->onCallEvent(std::move(payload));
            });
    connect(&_livekit_bridge,
            &LivekitBridge::roomJoined,
            this,
            [this]()
            {
                _call_coordinator->onLivekitRoomJoined();
            });
    connect(&_livekit_bridge,
            &LivekitBridge::remoteTrackReady,
            this,
            [this]()
            {
                _call_coordinator->onLivekitRemoteTrackReady();
            });
    connect(&_livekit_bridge,
            &LivekitBridge::roomDisconnected,
            this,
            [this](const QString& reason, bool recoverable)
            {
                _call_coordinator->onLivekitRoomDisconnected(reason, recoverable);
            });
    connect(&_livekit_bridge,
            &LivekitBridge::permissionError,
            this,
            [this](const QString& deviceType, const QString& message)
            {
                _call_coordinator->onLivekitPermissionError(deviceType, message);
            });
    connect(&_livekit_bridge,
            &LivekitBridge::mediaError,
            this,
            [this](const QString& message)
            {
                _call_coordinator->onLivekitMediaError(message);
            });
    connect(&_livekit_bridge,
            &LivekitBridge::reconnecting,
            this,
            [this](const QString& message)
            {
                _call_coordinator->onLivekitReconnecting(message);
            });
    connect(&_livekit_bridge,
            &LivekitBridge::bridgeLog,
            this,
            [](const QString& message)
            {
                qInfo().noquote() << "[livekit]" << message;
            });
    connect(chatDispatcher.get(),
            &ChatMessageDispatcher::sig_heartbeat_ack,
            this,
            [this](qint64 ackAtMs)
            {
                _chat_connection_coordinator->onHeartbeatAck(ackAtMs);
            });
    connect(&_apply_request_model,
            &ApplyRequestModel::unapprovedCountChanged,
            this,
            &AppController::pendingApplyChanged);
    connect(&_apply_request_model,
            &ApplyRequestModel::unapprovedCountChanged,
            this,
            &AppController::syncContactControllerState);
    connect(&_auth_view_model, &AuthViewModel::clearTipRequested, this, &AppController::clearTip);
    connect(&_auth_view_model, &AuthViewModel::saveLoginCredentialRequested, this, &AppController::saveLoginCredential);
    connect(&_auth_view_model, &AuthViewModel::loginRequested, this, &AppController::login);
    connect(&_auth_view_model, &AuthViewModel::registerCodeRequested, this, &AppController::requestRegisterCode);
    connect(&_auth_view_model, &AuthViewModel::registerUserRequested, this, &AppController::registerUser);
    connect(&_auth_view_model, &AuthViewModel::resetCodeRequested, this, &AppController::requestResetCode);
    connect(&_auth_view_model, &AuthViewModel::resetPasswordRequested, this, &AppController::resetPassword);
    connect(&_shell_view_model, &ShellViewModel::switchToLoginRequested, this, &AppController::switchToLogin);
    connect(&_shell_view_model, &ShellViewModel::switchToRegisterRequested, this, &AppController::switchToRegister);
    connect(&_shell_view_model, &ShellViewModel::switchToResetRequested, this, &AppController::switchToReset);
    connect(&_shell_view_model, &ShellViewModel::switchChatTabRequested, this, &AppController::switchChatTab);
    connect(&_shell_view_model,
            &ShellViewModel::beginPostLoginBootstrapRequested,
            this,
            &AppController::beginPostLoginBootstrap);
    connect(&_shell_view_model,
            &ShellViewModel::openExternalResourceRequested,
            this,
            &AppController::openExternalResource);
    _auth_view_model.syncTip(_shell_state.tipText, _shell_state.tipError);
    _auth_view_model.syncBusy(_shell_state.busy);
    _auth_view_model.syncRegisterSuccessPage(_shell_state.registerSuccessPage);
    _auth_view_model.syncRegisterCountdown(_shell_state.registerCountdown);
    _auth_view_model.syncRegisterCodeCooldownSeconds(_shell_state.registerCodeCooldownSeconds);
    _auth_view_model.syncRegisterCodeRequestPending(_shell_state.registerCodeRequestPending);
    _auth_view_model.syncLoginCredentialCacheJson(loginCredentialCacheJson());
    connect(&_call_controller, &CallController::startVoiceChatRequested, this, &AppController::startVoiceChat);
    connect(&_call_controller, &CallController::startVideoChatRequested, this, &AppController::startVideoChat);
    connect(&_call_controller, &CallController::acceptIncomingCallRequested, this, &AppController::acceptIncomingCall);
    connect(&_call_controller, &CallController::rejectIncomingCallRequested, this, &AppController::rejectIncomingCall);
    connect(&_call_controller, &CallController::endCurrentCallRequested, this, &AppController::endCurrentCall);
    connect(&_call_controller, &CallController::toggleCallMutedRequested, this, &AppController::toggleCallMuted);
    connect(&_call_controller, &CallController::toggleCallCameraRequested, this, &AppController::toggleCallCamera);
    syncCallControllerState();
    connect(&_profile_controller, &ProfileController::chooseAvatarRequested, this, &AppController::chooseAvatar);
    connect(&_profile_controller, &ProfileController::saveProfileRequested, this, &AppController::saveProfile);
    connect(&_profile_controller, &ProfileController::clearStatusRequested, this, &AppController::clearSettingsStatus);
    _profile_controller.syncStatus(_feature_status_state.settingsText, _feature_status_state.settingsError);
    connect(&_contact_controller,
            &ContactController::ensureContactsInitializedRequested,
            this,
            &AppController::ensureContactsInitialized);
    connect(&_contact_controller,
            &ContactController::ensureApplyInitializedRequested,
            this,
            &AppController::ensureApplyInitialized);
    connect(&_contact_controller,
            &ContactController::selectContactIndexRequested,
            this,
            &AppController::selectContactIndex);
    connect(&_contact_controller, &ContactController::searchUserRequested, this, &AppController::searchUser);
    connect(&_contact_controller,
            &ContactController::clearSearchStateRequested,
            this,
            &AppController::clearSearchState);
    connect(&_contact_controller,
            &ContactController::requestAddFriendRequested,
            this,
            &AppController::requestAddFriend);
    connect(&_contact_controller, &ContactController::approveFriendRequested, this, &AppController::approveFriend);
    connect(&_contact_controller, &ContactController::deleteFriendRequested, this, &AppController::deleteFriend);
    connect(&_contact_controller,
            &ContactController::showApplyRequestsRequested,
            this,
            &AppController::showApplyRequests);
    connect(&_contact_controller,
            &ContactController::jumpChatWithCurrentContactRequested,
            this,
            &AppController::jumpChatWithCurrentContact);
    connect(&_contact_controller,
            &ContactController::loadMoreContactsRequested,
            this,
            &AppController::loadMoreContacts);
    connect(&_contact_controller, &ContactController::clearAuthStatusRequested, this, &AppController::clearAuthStatus);
    connect(&_group_controller,
            &GroupController::ensureGroupsInitializedRequested,
            this,
            &AppController::ensureGroupsInitialized);
    connect(&_group_controller, &GroupController::selectGroupIndexRequested, this, &AppController::selectGroupIndex);
    connect(&_group_controller, &GroupController::refreshGroupListRequested, this, &AppController::refreshGroupList);
    connect(&_group_controller, &GroupController::createGroupRequested, this, &AppController::createGroup);
    connect(&_group_controller, &GroupController::inviteGroupMemberRequested, this, &AppController::inviteGroupMember);
    connect(&_group_controller, &GroupController::applyJoinGroupRequested, this, &AppController::applyJoinGroup);
    connect(&_group_controller, &GroupController::reviewGroupApplyRequested, this, &AppController::reviewGroupApply);
    connect(&_group_controller,
            &GroupController::sendGroupTextMessageRequested,
            this,
            &AppController::sendGroupTextMessage);
    connect(&_group_controller,
            &GroupController::sendGroupImageMessageRequested,
            this,
            &AppController::sendGroupImageMessage);
    connect(&_group_controller,
            &GroupController::sendGroupFileMessageRequested,
            this,
            &AppController::sendGroupFileMessage);
    connect(&_group_controller, &GroupController::editGroupMessageRequested, this, &AppController::editGroupMessage);
    connect(&_group_controller,
            &GroupController::revokeGroupMessageRequested,
            this,
            &AppController::revokeGroupMessage);
    connect(&_group_controller,
            &GroupController::forwardGroupMessageRequested,
            this,
            &AppController::forwardGroupMessage);
    connect(&_group_controller, &GroupController::loadGroupHistoryRequested, this, &AppController::loadGroupHistory);
    connect(&_group_controller,
            &GroupController::updateGroupAnnouncementRequested,
            this,
            &AppController::updateGroupAnnouncement);
    connect(&_group_controller, &GroupController::updateGroupIconRequested, this, &AppController::updateGroupIcon);
    connect(&_group_controller, &GroupController::setGroupAdminRequested, this, &AppController::setGroupAdmin);
    connect(&_group_controller, &GroupController::muteGroupMemberRequested, this, &AppController::muteGroupMember);
    connect(&_group_controller, &GroupController::kickGroupMemberRequested, this, &AppController::kickGroupMember);
    connect(&_group_controller, &GroupController::quitCurrentGroupRequested, this, &AppController::quitCurrentGroup);
    connect(&_group_controller,
            &GroupController::dissolveCurrentGroupRequested,
            this,
            &AppController::dissolveCurrentGroup);
    connect(&_group_controller, &GroupController::clearGroupStatusRequested, this, &AppController::clearGroupStatus);
    connect(&_chat_view_model,
            &ChatViewModel::ensureChatListInitializedRequested,
            this,
            &AppController::ensureChatListInitialized);
    connect(&_chat_view_model,
            &ChatViewModel::ensureGroupsInitializedRequested,
            this,
            &AppController::ensureGroupsInitialized);
    connect(&_chat_view_model, &ChatViewModel::selectDialogByUidRequested, this, &AppController::selectDialogByUid);
    connect(&_chat_view_model, &ChatViewModel::selectChatIndexRequested, this, &AppController::selectChatIndex);
    connect(&_chat_view_model, &ChatViewModel::selectGroupIndexRequested, this, &AppController::selectGroupIndex);
    connect(&_chat_view_model, &ChatViewModel::loadMoreChatsRequested, this, &AppController::loadMoreChats);
    connect(&_chat_view_model,
            &ChatViewModel::loadMorePrivateHistoryRequested,
            this,
            &AppController::loadMorePrivateHistory);
    connect(&_chat_view_model,
            &ChatViewModel::sendCurrentComposerPayloadRequested,
            this,
            &AppController::sendCurrentComposerPayload);
    connect(&_chat_view_model, &ChatViewModel::sendImageMessageRequested, this, &AppController::sendImageMessage);
    connect(&_chat_view_model, &ChatViewModel::sendFileMessageRequested, this, &AppController::sendFileMessage);
    connect(&_chat_view_model,
            &ChatViewModel::removePendingAttachmentRequested,
            this,
            &AppController::removePendingAttachment);
    connect(&_chat_view_model,
            &ChatViewModel::clearPendingAttachmentsRequested,
            this,
            &AppController::clearPendingAttachments);
    connect(&_chat_view_model, &ChatViewModel::updateCurrentDraftRequested, this, &AppController::updateCurrentDraft);
    connect(&_chat_view_model,
            &ChatViewModel::toggleCurrentDialogPinnedRequested,
            this,
            &AppController::toggleCurrentDialogPinned);
    connect(&_chat_view_model,
            &ChatViewModel::toggleCurrentDialogMutedRequested,
            this,
            &AppController::toggleCurrentDialogMuted);
    connect(&_chat_view_model,
            &ChatViewModel::toggleDialogPinnedByUidRequested,
            this,
            &AppController::toggleDialogPinnedByUid);
    connect(&_chat_view_model,
            &ChatViewModel::toggleDialogMutedByUidRequested,
            this,
            &AppController::toggleDialogMutedByUid);
    connect(&_chat_view_model, &ChatViewModel::markDialogReadByUidRequested, this, &AppController::markDialogReadByUid);
    connect(&_chat_view_model,
            &ChatViewModel::clearDialogDraftByUidRequested,
            this,
            &AppController::clearDialogDraftByUid);
    connect(&_chat_view_model, &ChatViewModel::beginReplyMessageRequested, this, &AppController::beginReplyMessage);
    connect(&_chat_view_model, &ChatViewModel::cancelReplyMessageRequested, this, &AppController::cancelReplyMessage);
    connect(&_chat_view_model, &ChatViewModel::startVoiceChatRequested, this, &AppController::startVoiceChat);
    connect(&_chat_view_model, &ChatViewModel::startVideoChatRequested, this, &AppController::startVideoChat);
    connect(this, &AppController::chatTabChanged, this, &AppController::syncChatViewModelState);
    connect(this, &AppController::currentChatPeerChanged, this, &AppController::syncChatViewModelState);
    connect(this, &AppController::currentDialogUidChanged, this, &AppController::syncChatViewModelState);
    connect(this, &AppController::currentGroupChanged, this, &AppController::syncChatViewModelState);
    connect(this, &AppController::chatLoadingMoreChanged, this, &AppController::syncChatViewModelState);
    connect(this, &AppController::canLoadMoreChatsChanged, this, &AppController::syncChatViewModelState);
    connect(this, &AppController::privateHistoryLoadingChanged, this, &AppController::syncChatViewModelState);
    connect(this, &AppController::canLoadMorePrivateHistoryChanged, this, &AppController::syncChatViewModelState);
    connect(this, &AppController::mediaUploadStateChanged, this, &AppController::syncChatViewModelState);
    connect(this, &AppController::currentDraftTextChanged, this, &AppController::syncChatViewModelState);
    connect(this, &AppController::currentPendingAttachmentsChanged, this, &AppController::syncChatViewModelState);
    connect(this, &AppController::currentDialogPinnedChanged, this, &AppController::syncChatViewModelState);
    connect(this, &AppController::currentDialogMutedChanged, this, &AppController::syncChatViewModelState);
    connect(this, &AppController::pendingReplyChanged, this, &AppController::syncChatViewModelState);
    connect(this, &AppController::lazyBootstrapStateChanged, this, &AppController::syncChatViewModelState);
    syncChatViewModelState();
    syncContactControllerState();
    syncGroupControllerState();
    syncShellViewModelState();

    connect(&_register_countdown_timer, &QTimer::timeout, this, &AppController::onRegisterCountdownTimeout);
    connect(&_register_code_cooldown_timer, &QTimer::timeout, this, &AppController::onRegisterCodeCooldownTimeout);
    connect(&_heartbeat_timer,
            &QTimer::timeout,
            this,
            [this]()
            {
                _chat_connection_coordinator->onHeartbeatTimeout();
            });
    _chat_login_timeout_timer.setSingleShot(true);
    _chat_login_timeout_timer.setInterval(15000);
    connect(&_chat_login_timeout_timer,
            &QTimer::timeout,
            this,
            [this]()
            {
                if (_chat_recovery_state.reconnecting && _page == ChatPage)
                {
                    _gateway.chatTransport()->CloseConnection();
                    return;
                }
                if (!_shell_state.busy || _page != LoginPage)
                {
                    return;
                }
                const bool fallbackAlreadyAttempted = _chat_recovery_state.loginTcpFallbackAttempted;
                _gateway.chatTransport()->CloseConnection();
                if (!fallbackAlreadyAttempted && _chat_recovery_state.loginTcpFallbackAttempted)
                {
                    return;
                }
                if (_chat_connection_coordinator->tryLoginFallbackToTcp(QStringLiteral("login_timeout")))
                {
                    return;
                }
                setBusy(false);
                setTip("聊天服务登录超时，请重试", true);
            });
}

AppController::Page AppController::page() const
{
    return _page;
}

AppController::~AppController() = default;

CallSessionModel* AppController::callSession()
{
    return &_call_session_model;
}

LivekitBridge* AppController::livekitBridge()
{
    return &_livekit_bridge;
}

AuthViewModel* AppController::auth()
{
    return &_auth_view_model;
}

ShellViewModel* AppController::shell()
{
    return &_shell_view_model;
}

ShellViewModel* AppController::shellViewModel()
{
    return &_shell_view_model;
}

AuthViewModel* AppController::authViewModel()
{
    return &_auth_view_model;
}

ChatViewModel* AppController::chat()
{
    return &_chat_view_model;
}

ChatViewModel* AppController::chatViewModel()
{
    return &_chat_view_model;
}

void AppController::syncChatViewModelState()
{
    _chat_view_model.syncDialogListModel(&_dialog_list_model);
    _chat_view_model.syncMessageModel(&_message_model);
    _chat_view_model.syncChatTab(static_cast<int>(_chat_tab));
    _chat_view_model.syncCurrentDialogUid(currentDialogUid());
    _chat_view_model.syncCurrentPeer(currentChatPeerName(), currentChatPeerIcon(), hasCurrentChat());
    _chat_view_model.syncCurrentGroup(hasCurrentGroup(), currentGroupRole());
    _chat_view_model.syncCurrentDraftText(currentDraftText());
    _chat_view_model.syncCurrentPendingAttachments(currentPendingAttachments());
    _chat_view_model.syncCurrentDialogPinned(currentDialogPinned());
    _chat_view_model.syncCurrentDialogMuted(currentDialogMuted());
    _chat_view_model.syncPendingReply(hasPendingReply(), replyTargetName(), replyPreviewText());
    _chat_view_model.syncDialogsReady(dialogsReady());
    _chat_view_model.syncChatLoadingMore(chatLoadingMore());
    _chat_view_model.syncCanLoadMoreChats(canLoadMoreChats());
    _chat_view_model.syncPrivateHistoryLoading(privateHistoryLoading());
    _chat_view_model.syncCanLoadMorePrivateHistory(canLoadMorePrivateHistory());
    _chat_view_model.syncMediaUpload(mediaUploadInProgress(), mediaUploadProgressText());
}

void AppController::syncContactControllerState()
{
    _contact_controller.syncModels(&_contact_list_model, &_search_result_model, &_apply_request_model);
    _contact_controller.syncContactPane(static_cast<int>(_contact_pane));
    _contact_controller.syncCurrentContact(_contact_state.uid,
                                           _contact_state.name,
                                           _contact_state.nick,
                                           _contact_state.icon,
                                           _contact_state.back,
                                           _contact_state.sex,
                                           _contact_state.userId);
    _contact_controller.syncSearchPending(_search_state.pending);
    _contact_controller.syncSearchStatus(_search_state.statusText, _search_state.statusError);
    _contact_controller.syncAuthStatus(_feature_status_state.authText, _feature_status_state.authError);
    _contact_controller.syncHasPendingApply(_apply_request_model.hasUnapproved());
    _contact_controller.syncContactLoadingMore(_loading_state.contactLoadingMore);
    _contact_controller.syncCanLoadMoreContacts(_loading_state.canLoadMoreContacts);
    _contact_controller.syncContactsReady(_bootstrap_state.contactsReady);
}

void AppController::syncGroupControllerState()
{
    _group_controller.syncModel(&_group_list_model);
    _group_controller.syncCurrentGroup(hasCurrentGroup(),
                                       currentGroupRole(),
                                       currentGroupName(),
                                       currentGroupCode(),
                                       currentGroupCanChangeInfo(),
                                       currentGroupCanDeleteMessages(),
                                       currentGroupCanInviteUsers(),
                                       currentGroupCanManageAdmins(),
                                       currentGroupCanPinMessages(),
                                       currentGroupCanBanUsers(),
                                       currentGroupCanManageTopics());
    _group_controller.syncGroupStatus(_feature_status_state.groupText, _feature_status_state.groupError);
    _group_controller.syncGroupsReady(_bootstrap_state.groupsReady);
}

void AppController::syncCallControllerState()
{
    _call_controller.syncSurface(&_call_session_model, &_livekit_bridge);
}

void AppController::syncShellViewModelState()
{
    _shell_view_model.syncPage(static_cast<int>(_page));
    _shell_view_model.syncChatTab(static_cast<int>(_chat_tab));
    _shell_view_model.syncCurrentUser(currentUserName(),
                                      currentUserNick(),
                                      currentUserIcon(),
                                      currentUserDesc(),
                                      currentUserId(),
                                      currentUserUid());
}

void AppController::clearTip()
{
    setTip("", false);
}

void AppController::openExternalResource(const QString& url)
{
    QString errorText;
    if (!LocalFilePickerService::openUrl(url, &errorText))
    {
        if (errorText.isEmpty())
        {
            errorText = "打开资源失败";
        }
        setTip(errorText, true);
    }
}

void AppController::searchUser(const QString& uidText)
{
    _contact_coordinator_shell->searchUser(uidText);
}

void AppController::clearSearchState()
{
    clearSearchResultOnly();
    setSearchPending(false);
    setSearchStatus("", false);
}

void AppController::requestAddFriend(int uid, const QString& bakName, const QVariantList& labels)
{
    _contact_coordinator_shell->requestAddFriend(uid, bakName, labels);
}

void AppController::approveFriend(int uid, const QString& backName, const QVariantList& labels)
{
    _contact_coordinator_shell->approveFriend(uid, backName, labels);
}

void AppController::clearAuthStatus()
{
    setAuthStatus("", false);
}

void AppController::startVoiceChat()
{
    _call_coordinator->startVoiceChat();
}

void AppController::startVideoChat()
{
    _call_coordinator->startVideoChat();
}

void AppController::acceptIncomingCall()
{
    _call_coordinator->acceptIncomingCall();
}

void AppController::rejectIncomingCall()
{
    _call_coordinator->rejectIncomingCall();
}

void AppController::endCurrentCall()
{
    _call_coordinator->endCurrentCall();
}

void AppController::toggleCallMuted()
{
    _call_coordinator->toggleCallMuted();
}

void AppController::toggleCallCamera()
{
    _call_coordinator->toggleCallCamera();
}
