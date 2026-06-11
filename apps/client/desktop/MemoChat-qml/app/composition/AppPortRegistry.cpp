#include "AppPortRegistry.h"

#include <utility>

AppPortRegistry::AppPortRegistry(AppPortRegistryContext context)
    : _constants(context.constants)
    , _queries(std::move(context.queries))
    , _actions(std::move(context.actions))
    , _events(std::move(context.events))
    , _async_receiver(context.refs.asyncReceiver)
    , _shell_state(context.refs.shellState)
    , _media_upload_state(context.refs.mediaUploadState)
    , _chat_state(context.refs.chatState)
    , _gateway(context.refs.gateway)
    , _features(context.refs.features)
    , _heartbeat_timer(context.refs.heartbeatTimer)
    , _chat_login_timeout_timer(context.refs.chatLoginTimeoutTimer)
    , _session_coordinator(context.refs.sessionCoordinator)
    , _media_coordinator(context.refs.mediaCoordinator)
    , _call_coordinator(context.refs.callCoordinator)
    , _chat_connection_coordinator(context.refs.chatConnectionCoordinator)
{
}

int AppPortRegistry::page() const
{
    return _queries.page();
}

bool AppPortRegistry::contactsReady() const
{
    return _queries.contactsReady();
}

bool AppPortRegistry::isChatTransportReady() const
{
    return _queries.isChatTransportReady();
}

bool AppPortRegistry::busy() const
{
    return _queries.busy();
}

int AppPortRegistry::currentUserUid() const
{
    return _queries.currentUserUid();
}

int AppPortRegistry::currentDialogUid() const
{
    return _queries.currentDialogUid();
}

qint64 AppPortRegistry::currentGroupId() const
{
    return _queries.currentGroupId();
}

bool AppPortRegistry::hasCurrentContact() const
{
    return _queries.hasCurrentContact();
}

int AppPortRegistry::currentContactUid() const
{
    return _queries.currentContactUid();
}

QString AppPortRegistry::currentContactName() const
{
    return _queries.currentContactName();
}

QString AppPortRegistry::currentContactIcon() const
{
    return _queries.currentContactIcon();
}

QString AppPortRegistry::currentUserName() const
{
    return _queries.currentUserName();
}

QString AppPortRegistry::currentUserNick() const
{
    return _queries.currentUserNick();
}

QString AppPortRegistry::currentUserIcon() const
{
    return _queries.currentUserIcon();
}

QString AppPortRegistry::currentUserDesc() const
{
    return _queries.currentUserDesc();
}

QString AppPortRegistry::currentUserId() const
{
    return _queries.currentUserId();
}

int AppPortRegistry::currentGroupRole() const
{
    return _queries.currentGroupRole();
}

bool AppPortRegistry::currentGroupCanChangeInfo() const
{
    return _queries.currentGroupCanChangeInfo();
}

bool AppPortRegistry::currentGroupCanManageAdmins() const
{
    return _queries.currentGroupCanManageAdmins();
}

QVariantList AppPortRegistry::currentPendingAttachments() const
{
    return _queries.currentPendingAttachments();
}

QString AppPortRegistry::normalizeIconPath(QString icon) const
{
    return _queries.normalizeIconPath(std::move(icon));
}

void AppPortRegistry::setPage(int page)
{
    _actions.setPage(page);
}

void AppPortRegistry::clearTip()
{
    _actions.clearTip();
}

void AppPortRegistry::switchToLogin()
{
    _actions.switchToLogin();
}

void AppPortRegistry::setTip(const QString& tip, bool isError)
{
    _actions.setTip(tip, isError);
}

void AppPortRegistry::setBusy(bool value)
{
    _actions.setBusy(value);
}

void AppPortRegistry::setSearchPending(bool pending)
{
    _actions.setSearchPending(pending);
}

void AppPortRegistry::setSearchStatus(const QString& text, bool isError)
{
    _actions.setSearchStatus(text, isError);
}

void AppPortRegistry::setAuthStatus(const QString& text, bool isError)
{
    _actions.setAuthStatus(text, isError);
}

void AppPortRegistry::setSettingsStatus(const QString& text, bool isError)
{
    _actions.setSettingsStatus(text, isError);
}

void AppPortRegistry::setGroupStatus(const QString& text, bool isError)
{
    _actions.setGroupStatus(text, isError);
}

void AppPortRegistry::setChatLoadingMore(bool loading)
{
    _actions.setChatLoadingMore(loading);
}

void AppPortRegistry::setPrivateHistoryLoading(bool loading)
{
    _actions.setPrivateHistoryLoading(loading);
}

void AppPortRegistry::setCanLoadMorePrivateHistory(bool canLoad)
{
    _actions.setCanLoadMorePrivateHistory(canLoad);
}

void AppPortRegistry::setContactLoadingMore(bool loading)
{
    _actions.setContactLoadingMore(loading);
}

void AppPortRegistry::setDialogsReady(bool ready)
{
    _actions.setDialogsReady(ready);
}

void AppPortRegistry::setContactsReady(bool ready)
{
    _actions.setContactsReady(ready);
}

void AppPortRegistry::setGroupsReady(bool ready)
{
    _actions.setGroupsReady(ready);
}

void AppPortRegistry::setApplyReady(bool ready)
{
    _actions.setApplyReady(ready);
}

void AppPortRegistry::setContactPane(int pane)
{
    _actions.setContactPane(pane);
}

void AppPortRegistry::setCurrentContact(int uid,
                                        const QString& name,
                                        const QString& nick,
                                        const QString& icon,
                                        const QString& back,
                                        int sex,
                                        const QString& userId)
{
    _actions.setCurrentContact(uid, name, nick, icon, back, sex, userId);
}

void AppPortRegistry::setCurrentChatPeerName(const QString& name)
{
    _actions.setCurrentChatPeerName(name);
}

void AppPortRegistry::setCurrentChatPeerIcon(const QString& icon)
{
    _actions.setCurrentChatPeerIcon(icon);
}

void AppPortRegistry::setCurrentDraftText(const QString& text)
{
    _actions.setCurrentDraftText(text);
}

void AppPortRegistry::setCurrentPendingAttachments(const QVariantList& attachments)
{
    _actions.setCurrentPendingAttachments(attachments);
}

void AppPortRegistry::setCurrentDialogPinned(bool pinned)
{
    _actions.setCurrentDialogPinned(pinned);
}

void AppPortRegistry::setCurrentDialogMuted(bool muted)
{
    _actions.setCurrentDialogMuted(muted);
}

void AppPortRegistry::setPendingReplyContext(const QString& msgId,
                                             const QString& senderName,
                                             const QString& previewText)
{
    _actions.setPendingReplyContext(msgId, senderName, previewText);
}

void AppPortRegistry::setMediaUploadInProgress(bool inProgress)
{
    _actions.setMediaUploadInProgress(inProgress);
}

void AppPortRegistry::setMediaUploadProgressText(const QString& text)
{
    _actions.setMediaUploadProgressText(text);
}

void AppPortRegistry::applyCurrentUserProfile(const QJsonObject& profile, bool preserveExistingIcon)
{
    _actions.applyCurrentUserProfileObject(profile, preserveExistingIcon);
}

void AppPortRegistry::applyCurrentUserProfile(int uid,
                                              const QString& name,
                                              const QString& nick,
                                              const QString& icon,
                                              const QString& desc,
                                              const QString& userId,
                                              int sex,
                                              bool preserveExistingIcon)
{
    _actions.applyCurrentUserProfileValues(uid, name, nick, icon, desc, userId, sex, preserveExistingIcon);
}

void AppPortRegistry::ensureChatListInitialized()
{
    _actions.ensureChatListInitialized();
}

void AppPortRegistry::ensureApplyInitialized()
{
    _actions.ensureApplyInitialized();
}

void AppPortRegistry::bootstrapDialogs()
{
    _actions.bootstrapDialogs();
}

void AppPortRegistry::bootstrapGroups()
{
    _actions.bootstrapGroups();
}

void AppPortRegistry::requestRelationBootstrap()
{
    _actions.requestRelationBootstrap();
}

void AppPortRegistry::refreshContactLoadMoreState()
{
    _actions.refreshContactLoadMoreState();
}

void AppPortRegistry::refreshDialogModelIncremental()
{
    _actions.refreshDialogModelIncremental();
}

void AppPortRegistry::flushIncomingMessageRouter()
{
    _actions.flushIncomingMessageRouter();
}

void AppPortRegistry::loadDraftStore(int ownerUid)
{
    _actions.loadDraftStore(ownerUid);
}

void AppPortRegistry::startPendingAttachmentRunner()
{
    _actions.startPendingAttachmentRunner();
}

void AppPortRegistry::addPendingAttachments(const QVariantList& attachments)
{
    _actions.addPendingAttachments(attachments);
}

void AppPortRegistry::removePendingAttachmentById(const QString& attachmentId)
{
    _actions.removePendingAttachmentById(attachmentId);
}

void AppPortRegistry::jumpChatWithCurrentContact()
{
    _actions.jumpChatWithCurrentContact();
}

void AppPortRegistry::selectGroupIndex(int index)
{
    _actions.selectGroupIndex(index);
}

void AppPortRegistry::syncShellViewModelState()
{
    _actions.syncShellViewModelState();
}

void AppPortRegistry::emitCurrentDialogUidChangedIfNeeded()
{
    _actions.emitCurrentDialogUidChangedIfNeeded();
}

void AppPortRegistry::pendingApplyChanged()
{
    _events.pendingApplyChanged();
}

void AppPortRegistry::canLoadMoreChatsChanged()
{
    _events.canLoadMoreChatsChanged();
}

void AppPortRegistry::currentGroupChanged()
{
    _events.currentGroupChanged();
}

void AppPortRegistry::currentUserChanged()
{
    _events.currentUserChanged();
}
