#include "AppController.h"
#include "AppChatConnectionCoordinator.h"
#include "AppChatDispatcherEventRouter.h"
#include "AppCoordinators.h"
#include "AppHttpEventRouter.h"
#include "AppPortRegistry.h"
#include "LocalFilePickerService.h"

#include <utility>

AppController::AppController(QObject* parent)
    : QObject(parent)
    , _shell_state(LoginPage, ChatTabPage)
    , _features(_gateway, this)
{
    AppPortRegistryQueries queries;
    queries.page = [this]()
    {
        return page();
    };
    queries.contactsReady = [this]()
    {
        return contactsReady();
    };
    queries.isChatTransportReady = [this]()
    {
        return isChatTransportReady();
    };
    queries.busy = [this]()
    {
        return busy();
    };
    queries.currentUserUid = [this]()
    {
        return currentUserUid();
    };
    queries.currentDialogUid = [this]()
    {
        return currentDialogUid();
    };
    queries.currentGroupId = [this]()
    {
        return currentGroupId();
    };
    queries.hasCurrentContact = [this]()
    {
        return hasCurrentContact();
    };
    queries.currentContactUid = [this]()
    {
        return currentContactUid();
    };
    queries.currentContactName = [this]()
    {
        return currentContactName();
    };
    queries.currentContactIcon = [this]()
    {
        return currentContactIcon();
    };
    queries.currentUserName = [this]()
    {
        return currentUserName();
    };
    queries.currentUserNick = [this]()
    {
        return currentUserNick();
    };
    queries.currentUserIcon = [this]()
    {
        return currentUserIcon();
    };
    queries.currentUserDesc = [this]()
    {
        return currentUserDesc();
    };
    queries.currentUserId = [this]()
    {
        return currentUserId();
    };
    queries.currentGroupRole = [this]()
    {
        return currentGroupRole();
    };
    queries.currentGroupCanChangeInfo = [this]()
    {
        return currentGroupCanChangeInfo();
    };
    queries.currentGroupCanManageAdmins = [this]()
    {
        return currentGroupCanManageAdmins();
    };
    queries.currentPendingAttachments = [this]()
    {
        return currentPendingAttachments();
    };
    queries.normalizeIconPath = [this](QString icon)
    {
        return normalizeIconPath(std::move(icon));
    };

    AppPortRegistryActions actions;
    actions.setPage = [this](int pageValue)
    {
        setPage(static_cast<Page>(pageValue));
    };
    actions.clearTip = [this]()
    {
        clearTip();
    };
    actions.switchToLogin = [this]()
    {
        switchToLogin();
    };
    actions.setTip = [this](const QString& tip, bool isError)
    {
        setTip(tip, isError);
    };
    actions.setBusy = [this](bool value)
    {
        setBusy(value);
    };
    actions.setSearchPending = [this](bool pending)
    {
        setSearchPending(pending);
    };
    actions.setSearchStatus = [this](const QString& text, bool isError)
    {
        setSearchStatus(text, isError);
    };
    actions.setAuthStatus = [this](const QString& text, bool isError)
    {
        setAuthStatus(text, isError);
    };
    actions.setSettingsStatus = [this](const QString& text, bool isError)
    {
        setSettingsStatus(text, isError);
    };
    actions.setGroupStatus = [this](const QString& text, bool isError)
    {
        setGroupStatus(text, isError);
    };
    actions.setChatLoadingMore = [this](bool loading)
    {
        setChatLoadingMore(loading);
    };
    actions.setPrivateHistoryLoading = [this](bool loading)
    {
        setPrivateHistoryLoading(loading);
    };
    actions.setCanLoadMorePrivateHistory = [this](bool canLoad)
    {
        setCanLoadMorePrivateHistory(canLoad);
    };
    actions.setContactLoadingMore = [this](bool loading)
    {
        setContactLoadingMore(loading);
    };
    actions.setDialogsReady = [this](bool ready)
    {
        setDialogsReady(ready);
    };
    actions.setContactsReady = [this](bool ready)
    {
        setContactsReady(ready);
    };
    actions.setGroupsReady = [this](bool ready)
    {
        setGroupsReady(ready);
    };
    actions.setApplyReady = [this](bool ready)
    {
        setApplyReady(ready);
    };
    actions.setContactPane = [this](int pane)
    {
        setContactPane(static_cast<ContactPane>(pane));
    };
    actions.setCurrentContact = [this](int uid,
                                       const QString& name,
                                       const QString& nick,
                                       const QString& icon,
                                       const QString& back,
                                       int sex,
                                       const QString& userId)
    {
        setCurrentContact(uid, name, nick, icon, back, sex, userId);
    };
    actions.setCurrentChatPeerName = [this](const QString& name)
    {
        setCurrentChatPeerName(name);
    };
    actions.setCurrentChatPeerIcon = [this](const QString& icon)
    {
        setCurrentChatPeerIcon(icon);
    };
    actions.setCurrentDraftText = [this](const QString& text)
    {
        setCurrentDraftText(text);
    };
    actions.setCurrentPendingAttachments = [this](const QVariantList& attachments)
    {
        setCurrentPendingAttachments(attachments);
    };
    actions.setCurrentDialogPinned = [this](bool pinned)
    {
        setCurrentDialogPinned(pinned);
    };
    actions.setCurrentDialogMuted = [this](bool muted)
    {
        setCurrentDialogMuted(muted);
    };
    actions.setPendingReplyContext = [this](const QString& msgId, const QString& senderName, const QString& previewText)
    {
        setPendingReplyContext(msgId, senderName, previewText);
    };
    actions.setMediaUploadInProgress = [this](bool inProgress)
    {
        setMediaUploadInProgress(inProgress);
    };
    actions.setMediaUploadProgressText = [this](const QString& text)
    {
        setMediaUploadProgressText(text);
    };
    actions.applyCurrentUserProfileObject = [this](const QJsonObject& profile, bool preserveExistingIcon)
    {
        applyCurrentUserProfile(profile, preserveExistingIcon);
    };
    actions.applyCurrentUserProfileValues = [this](int uid,
                                                   const QString& name,
                                                   const QString& nick,
                                                   const QString& icon,
                                                   const QString& desc,
                                                   const QString& userId,
                                                   int sex,
                                                   bool preserveExistingIcon)
    {
        applyCurrentUserProfile(uid, name, nick, icon, desc, userId, sex, preserveExistingIcon);
    };
    actions.syncCurrentUserProfileState = [this](const ProfileAppliedUserState& user)
    {
        syncCurrentUserProfileState(user);
    };
    actions.ensureChatListInitialized = [this]()
    {
        ensureChatListInitialized();
    };
    actions.ensureApplyInitialized = [this]()
    {
        ensureApplyInitialized();
    };
    actions.bootstrapDialogs = [this]()
    {
        bootstrapDialogs();
    };
    actions.bootstrapGroups = [this]()
    {
        bootstrapGroups();
    };
    actions.requestRelationBootstrap = [this]()
    {
        requestRelationBootstrap();
    };
    actions.refreshContactLoadMoreState = [this]()
    {
        refreshContactLoadMoreState();
    };
    actions.refreshDialogModelIncremental = [this]()
    {
        refreshDialogModelIncremental();
    };
    actions.flushIncomingMessageRouter = [this]()
    {
        flushIncomingMessageRouter();
    };
    actions.loadDraftStore = [this](int ownerUid)
    {
        loadDraftStore(ownerUid);
    };
    actions.startPendingAttachmentRunner = [this]()
    {
        startPendingAttachmentRunner();
    };
    actions.addPendingAttachments = [this](const QVariantList& attachments)
    {
        addPendingAttachments(attachments);
    };
    actions.removePendingAttachmentById = [this](const QString& attachmentId)
    {
        removePendingAttachmentById(attachmentId);
    };
    actions.jumpChatWithCurrentContact = [this]()
    {
        jumpChatWithCurrentContact();
    };
    actions.selectGroupIndex = [this](int index)
    {
        selectGroupIndex(index);
    };
    actions.syncShellViewModelState = [this]()
    {
        syncShellViewModelState();
    };
    actions.emitCurrentDialogUidChangedIfNeeded = [this]()
    {
        emitCurrentDialogUidChangedIfNeeded();
    };

    AppPortRegistryContext context{AppPortRegistryRefs{*this,
                                                       _shell_state,
                                                       _media_upload_state,
                                                       _chat_state,
                                                       _gateway,
                                                       _features,
                                                       _heartbeat_timer,
                                                       _chat_login_timeout_timer,
                                                       _session_coordinator,
                                                       _media_coordinator,
                                                       _call_coordinator,
                                                       _chat_connection_coordinator},
                                   AppPortRegistryConstants{LoginPage, ChatPage, ApplyRequestPane},
                                   std::move(queries),
                                   std::move(actions)};
    _port_registry = std::make_unique<AppPortRegistry>(std::move(context));
    bindAppControllerPorts();

    bindAppControllerSignals();
    syncCallControllerState();
    bindChatFeatureController();
    syncChatViewModelState();
    syncContactControllerState();
    syncGroupControllerState();
    syncShellViewModelState();
}

AppController::Page AppController::page() const
{
    return static_cast<Page>(_shell_state.page());
}

AppController::~AppController() = default;

ShellViewModel* AppController::shellViewModel()
{
    return &_features.shellViewModel;
}

AuthViewModel* AppController::authViewModel()
{
    return &_features.authViewModel;
}

ChatViewModel* AppController::chatViewModel()
{
    return &_features.chatViewModel;
}

void AppController::syncChatViewModelState()
{
    _features.chatFeatureController.syncViewModelState();
}

void AppController::syncContactControllerState()
{
}

void AppController::syncGroupControllerState()
{
    _features.groupController.syncGroupsReady(_shell_state.bootstrapState().groupsReady);
}

void AppController::syncCallControllerState()
{
}

void AppController::syncShellViewModelState()
{
    _features.shellViewModel.syncPage(_shell_state.page());
    _features.shellViewModel.syncChatTab(_shell_state.chatTab());
    _features.shellViewModel.syncCurrentUser(currentUserName(),
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
