#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H

#include <QObject>
#include <QTimer>
#include <QJsonObject>
#include <memory>
#include <QVariantList>
#include <QHash>
#include "global.h"
#include "AppFeatureRegistry.h"
#include "AppControllerConnectionState.h"
#include "AppControllerRuntimeState.h"
#include "AppControllerUserState.h"
#include "AppShellStateController.h"
#include "ClientGateway.h"

class AppSessionCoordinator;
class MediaCoordinator;
class CallCoordinator;
class AppChatConnectionCoordinator;
class AppChatDispatcherEventRouter;
class AppHttpEventRouter;
class AppPortRegistry;
class AgentController;
class AgentMessageModel;
class AuthViewModel;
class CallController;
class ChatViewModel;
class ContactController;
class GroupController;
class MomentsController;
class MomentsModel;
class PetController;
class ProfileController;
class R18Controller;
class ShellViewModel;
struct AddFriendApply;
struct AuthInfo;
struct AuthRsp;
struct GroupChatMsg;
struct SearchInfo;
struct TextChatMsg;
struct ChatDialogListAppPort;
struct ChatDialogListResponseContext;
struct PrivateChatEventContext;
struct PrivateChatEventDependencies;
struct GroupConversationContext;
struct GroupConversationDependencies;
namespace memochat::app
{
struct AppChatDispatcherGroupResponseHandlers;
struct ChatDialogSelectionActions;
struct ContactEventActions;
struct GroupManagementEventEffectActions;
struct GroupManagementResponseEffectActions;
struct IncomingMessageRouterDispatchActions;
struct IncomingMessageRouterReadinessInputs;
} // namespace memochat::app
namespace memochat::group
{
struct GroupManagementEventContext;
} // namespace memochat::group

class AppController : public QObject
{
    Q_OBJECT

public:
    enum Page
    {
        LoginPage = 0,
        RegisterPage = 1,
        ResetPage = 2,
        ChatPage = 3
    };
    Q_ENUM(Page)

    enum ChatTab
    {
        ChatTabPage = 0,
        ContactTabPage = 1,
        SettingsTabPage = 2,
        MomentsTabPage = 3,
        AgentTabPage = 4,
        Live2DTabPage = 5
    };
    Q_ENUM(ChatTab)

    enum ContactPane
    {
        ApplyRequestPane = 0,
        FriendInfoPane = 1
    };
    Q_ENUM(ContactPane)

    explicit AppController(QObject* parent = nullptr);
    ~AppController();

    MomentsModel* momentsModel() const;
    MomentsController* momentsController() const;
    AgentController* agentController() const;
    AgentMessageModel* agentMessageModel() const;
    PetController* petController() const;
    R18Controller* r18Controller() const;
    ShellViewModel* shellViewModel();
    AuthViewModel* authViewModel();
    ChatViewModel* chatViewModel();
    ContactController* contactController();
    GroupController* groupController();
    ProfileController* profileController();
    CallController* callController();

signals:
    void pageChanged();
    void tipChanged();
    void busyChanged();
    void chatTabChanged();
    void contactPaneChanged();
    void currentContactChanged();
    void currentUserChanged();
    void currentChatPeerChanged();
    void currentDialogUidChanged();
    void searchPendingChanged();
    void searchStatusChanged();
    void pendingApplyChanged();
    void chatLoadingMoreChanged();
    void canLoadMoreChatsChanged();
    void privateHistoryLoadingChanged();
    void canLoadMorePrivateHistoryChanged();
    void contactLoadingMoreChanged();
    void canLoadMoreContactsChanged();
    void authStatusChanged();
    void settingsStatusChanged();
    void currentGroupChanged();
    void groupStatusChanged();
    void mediaUploadStateChanged();
    void currentDraftTextChanged();
    void currentPendingAttachmentsChanged();
    void currentDialogPinnedChanged();
    void currentDialogMutedChanged();
    void pendingReplyChanged();
    void lazyBootstrapStateChanged();
    void groupCreated(qint64 groupId);

private:
    Page page() const;
    QString tipText() const;
    bool tipError() const;
    bool busy() const;
    ChatTab chatTab() const;
    ContactPane contactPane() const;
    QString currentUserName() const;
    QString currentUserNick() const;
    QString currentUserIcon() const;
    QString currentUserDesc() const;
    QString currentUserId() const;
    int currentUserUid() const;
    QString currentContactName() const;
    QString currentContactNick() const;
    QString currentContactIcon() const;
    QString currentContactBack() const;
    int currentContactSex() const;
    QString currentContactUserId() const;
    int currentContactUid() const;
    bool hasCurrentContact() const;
    QString currentChatPeerName() const;
    QString currentChatPeerIcon() const;
    bool hasCurrentChat() const;
    bool searchPending() const;
    QString searchStatusText() const;
    bool searchStatusError() const;
    bool hasPendingApply() const;
    bool chatLoadingMore() const;
    bool canLoadMoreChats() const;
    bool privateHistoryLoading() const;
    bool canLoadMorePrivateHistory() const;
    bool contactLoadingMore() const;
    bool canLoadMoreContacts() const;
    bool mediaUploadInProgress() const;
    QString mediaUploadProgressText() const;
    QString currentDraftText() const;
    QVariantList currentPendingAttachments() const;
    bool hasPendingAttachments() const;
    bool currentDialogPinned() const;
    bool currentDialogMuted() const;
    bool hasPendingReply() const;
    QString replyTargetName() const;
    QString replyPreviewText() const;
    bool dialogsReady() const;
    bool contactsReady() const;
    bool groupsReady() const;
    bool applyReady() const;
    bool chatShellBusy() const;

    void switchToLogin();
    void switchToRegister();
    void switchToReset();
    void switchChatTab(int tab);
    void ensureContactsInitialized();
    void ensureGroupsInitialized();
    void ensureApplyInitialized();
    void ensureChatListInitialized();
    void clearTip();
    void selectChatIndex(int index);
    void selectGroupIndex(int index);
    void selectDialogByUid(int uid);
    void jumpChatWithCurrentContact();
    void loadMoreChats();
    void openExternalResource(const QString& url);
    void refreshGroupList();
    void beginPostLoginBootstrap();

    bool isChatTransportReady() const;
    void addPendingAttachments(const QVariantList& attachments);
    void removePendingAttachmentById(const QString& attachmentId, int dialogUid = 0);
    void setCurrentPendingAttachments(const QVariantList& attachments);
    void startPendingAttachmentRunner();
    void setTip(const QString& tip, bool isError);
    void setBusy(bool value);
    void setPage(Page newPage);
    QString normalizeIconPath(QString icon) const;
    void applyCurrentUserProfile(const QJsonObject& profile, bool preserveExistingIcon);
    void applyCurrentUserProfile(int uid,
                                 const QString& name,
                                 const QString& nick,
                                 const QString& icon,
                                 const QString& desc,
                                 const QString& userId,
                                 int sex,
                                 bool preserveExistingIcon);
    void syncCurrentUserProfileState(const ProfileAppliedUserState& user);
    void refreshFriendModels();
    void refreshGroupModel();
    void refreshDialogModel();
    void refreshDialogModelIncremental();
    void requestDialogList();
    void requestRelationBootstrap();
    void bootstrapDialogs();
    void bootstrapContacts();
    void bootstrapGroups();
    void setDialogsReady(bool ready);
    void setContactsReady(bool ready);
    void setGroupsReady(bool ready);
    void setApplyReady(bool ready);
    void refreshChatLoadMoreState();
    void refreshContactLoadMoreState();
    void loadCurrentChatMessages();
    void selectGroupByDialogUid(int dialogUid, qint64 groupId);
    void requestPrivateHistory(int peerUid, qint64 beforeTs, const QString& beforeMsgId = QString());
    void requestPrivateHistoryForBootstrap(int peerUid);
    void requestGroupHistoryForBootstrap(qint64 groupId);
    void setContactPane(ContactPane pane);
    void setCurrentContact(int uid,
                           const QString& name,
                           const QString& nick,
                           const QString& icon,
                           const QString& back,
                           int sex,
                           const QString& userId = QString());
    void setCurrentChatPeerName(const QString& name);
    void setCurrentChatPeerIcon(const QString& icon);
    void selectChatByUid(int uid);
    void setSearchPending(bool pending);
    void setSearchStatus(const QString& text, bool isError);
    void clearSearchResultOnly();
    void setChatLoadingMore(bool loading);
    void setPrivateHistoryLoading(bool loading);
    void setCanLoadMorePrivateHistory(bool canLoad);
    void setContactLoadingMore(bool loading);
    void setAuthStatus(const QString& text, bool isError);
    void setSettingsStatus(const QString& text, bool isError);
    void setCurrentGroup(qint64 groupId, const QString& name, const QString& groupCode = QString());
    void setGroupStatus(const QString& text, bool isError);
    void setMediaUploadInProgress(bool inProgress);
    void setMediaUploadProgressText(const QString& text);
    void setCurrentDraftText(const QString& text);
    void setCurrentDialogPinned(bool pinned);
    void setCurrentDialogMuted(bool muted);
    void setPendingReplyContext(const QString& msgId, const QString& senderName, const QString& previewText);
    int currentDialogUid() const;
    qint64 currentGroupId() const;
    bool hasCurrentGroup() const;
    int currentGroupRole() const;
    QString currentGroupName() const;
    QString currentGroupCode() const;
    bool currentGroupCanChangeInfo() const;
    bool currentGroupCanManageAdmins() const;
    void emitCurrentDialogUidChangedIfNeeded();
    qint64 groupIdForDialogUid(int dialogUid) const;
    qint64 currentGroupPermissionBitsRaw() const;
    bool hasCurrentGroupPermission(qint64 permissionBit) const;
    void clearPrivateHistoryFeatureState();
    void resetGroupConversationFeatureState();
    memochat::app::ChatDialogSelectionActions chatDialogSelectionActions();
    ChatDialogListResponseContext dialogListResponseContext() const;
    ChatDialogListAppPort dialogListAppPort();
    PrivateChatEventContext privateChatEventContext(int selfUid) const;
    memochat::app::ContactEventActions contactEventActions();
    GroupConversationContext groupConversationContext(int selfUid) const;
    memochat::group::GroupManagementEventContext groupManagementEventContext() const;
    void syncCurrentDialogDraft();
    void clearCurrentGroupConversation(qint64 groupId);
    void loadDraftStore(int ownerUid);
    void saveDraftStore(int ownerUid) const;
    void syncCurrentPendingAttachments();
    memochat::app::IncomingMessageRouterReadinessInputs incomingMessageReadinessInputs() const;
    memochat::app::IncomingMessageRouterDispatchActions incomingMessageDispatchActions();
    void flushIncomingMessageRouter();
    void clearIncomingMessageRouter();
    void applyTextChatMsg(std::shared_ptr<TextChatMsg> msg);
    void applyGroupChatMsg(std::shared_ptr<GroupChatMsg> msg);
    void runPostLoginBootstrap();
    memochat::app::GroupManagementEventEffectActions groupManagementEventEffectActions();
    memochat::app::GroupManagementResponseEffectActions groupManagementResponseEffectActions();
    memochat::app::AppChatDispatcherGroupResponseHandlers groupResponseHandlers();
    bool handleGroupRspError(ReqId reqId, int error, const QJsonObject& payload);
    void handleGroupManagementRsp(ReqId reqId, const QJsonObject& payload);
    void handleGroupHistoryRsp(const QJsonObject& payload);
    void handleGroupMessageAckRsp(ReqId reqId, const QJsonObject& payload);
    void handleGroupMessageMutationRsp(ReqId reqId, const QJsonObject& payload);
    void handlePrivateMessageMutationRsp(ReqId reqId, const QJsonObject& payload);
    void handlePrivateForwardRsp(const QJsonObject& payload);
    void handleDialogMetaRsp(ReqId reqId, const QJsonObject& payload);
    void bindAppControllerPorts();
    void bindAppControllerSignals();
    void bindAppHttpSignals();
    void bindAppChatTransportSignals();
    void bindAppChatDispatcherSignals();
    void bindAppCallSignals();
    void bindAppFeatureFacadeSignals();
    void bindAppShellSignals();
    void bindAppChatProjectionSignals();
    void bindAppTimerSignals();
    void bindChatFeatureController();
    void bindChatFeatureProjectionPort();
    void bindChatFeatureDialogPorts();
    void bindChatFeatureHistoryPorts();
    void bindChatFeatureGroupPorts();
    void bindChatFeatureMediaPorts();
    void bindChatFeatureSendPorts();
    void bindChatFeatureReadMutationPorts();
    void syncChatViewModelState();
    void syncContactControllerState();
    void syncGroupControllerState();
    void syncCallControllerState();
    void syncShellViewModelState();

    AppShellStateController _shell_state;
    AppMediaUploadRuntimeState _media_upload_state;

    AppCurrentChatState _chat_state;

    ClientGateway _gateway;
    AppFeatureRegistry _features;
    QTimer _heartbeat_timer;
    QTimer _chat_login_timeout_timer;

    std::unique_ptr<AppSessionCoordinator> _session_coordinator;
    std::unique_ptr<MediaCoordinator> _media_coordinator;
    std::unique_ptr<CallCoordinator> _call_coordinator;
    std::unique_ptr<AppChatConnectionCoordinator> _chat_connection_coordinator;
    std::unique_ptr<AppChatDispatcherEventRouter> _chat_dispatcher_event_router;
    std::unique_ptr<AppHttpEventRouter> _http_event_router;
    std::unique_ptr<AppPortRegistry> _port_registry;
};

#endif // APPCONTROLLER_H
