#pragma once

#include "AppControllerRuntimeState.h"
#include "AppControllerUserState.h"
#include "AppFeatureRegistry.h"
#include "AppShellStateController.h"
#include "ClientGateway.h"

#include <QJsonObject>
#include <QString>
#include <QVariantList>
#include <functional>
#include <memory>

class AppChatConnectionCoordinator;
class AppSessionCoordinator;
class CallCoordinator;
class QObject;
class MediaCoordinator;
class QTimer;
struct ProfileAppliedUserState;
struct PostLoginBootstrapPort;
struct RegisterCountdownPort;
struct RelationBootstrapPort;
struct SessionAuthPort;
struct SessionLogoutPort;

struct AppPortRegistryRefs
{
    QObject& asyncReceiver;
    AppShellStateController& shellState;
    AppMediaUploadRuntimeState& mediaUploadState;
    AppCurrentChatState& chatState;
    ClientGateway& gateway;
    AppFeatureRegistry& features;
    QTimer& heartbeatTimer;
    QTimer& chatLoginTimeoutTimer;
    std::unique_ptr<AppSessionCoordinator>& sessionCoordinator;
    std::unique_ptr<MediaCoordinator>& mediaCoordinator;
    std::unique_ptr<CallCoordinator>& callCoordinator;
    std::unique_ptr<AppChatConnectionCoordinator>& chatConnectionCoordinator;
};

struct AppPortRegistryConstants
{
    int loginPage = 0;
    int chatPage = 0;
    int applyRequestPane = 0;
};

struct AppPortRegistryQueries
{
    std::function<int()> page;
    std::function<bool()> contactsReady;
    std::function<bool()> isChatTransportReady;
    std::function<bool()> busy;
    std::function<int()> currentUserUid;
    std::function<int()> currentDialogUid;
    std::function<qint64()> currentGroupId;
    std::function<bool()> hasCurrentContact;
    std::function<int()> currentContactUid;
    std::function<QString()> currentContactName;
    std::function<QString()> currentContactIcon;
    std::function<QString()> currentUserName;
    std::function<QString()> currentUserNick;
    std::function<QString()> currentUserIcon;
    std::function<QString()> currentUserDesc;
    std::function<QString()> currentUserId;
    std::function<int()> currentGroupRole;
    std::function<bool()> currentGroupCanChangeInfo;
    std::function<bool()> currentGroupCanManageAdmins;
    std::function<QVariantList()> currentPendingAttachments;
    std::function<QString(QString)> normalizeIconPath;
};

struct AppPortRegistryActions
{
    std::function<void(int)> setPage;
    std::function<void()> clearTip;
    std::function<void()> switchToLogin;
    std::function<void(const QString&, bool)> setTip;
    std::function<void(bool)> setBusy;
    std::function<void(bool)> setSearchPending;
    std::function<void(const QString&, bool)> setSearchStatus;
    std::function<void(const QString&, bool)> setAuthStatus;
    std::function<void(const QString&, bool)> setSettingsStatus;
    std::function<void(const QString&, bool)> setGroupStatus;
    std::function<void(bool)> setChatLoadingMore;
    std::function<void(bool)> setPrivateHistoryLoading;
    std::function<void(bool)> setCanLoadMorePrivateHistory;
    std::function<void(bool)> setContactLoadingMore;
    std::function<void(bool)> setDialogsReady;
    std::function<void(bool)> setContactsReady;
    std::function<void(bool)> setGroupsReady;
    std::function<void(bool)> setApplyReady;
    std::function<void(int)> setContactPane;
    std::function<void(int, const QString&, const QString&, const QString&, const QString&, int, const QString&)>
        setCurrentContact;
    std::function<void(const QString&)> setCurrentChatPeerName;
    std::function<void(const QString&)> setCurrentChatPeerIcon;
    std::function<void(const QString&)> setCurrentDraftText;
    std::function<void(const QVariantList&)> setCurrentPendingAttachments;
    std::function<void(bool)> setCurrentDialogPinned;
    std::function<void(bool)> setCurrentDialogMuted;
    std::function<void(const QString&, const QString&, const QString&)> setPendingReplyContext;
    std::function<void(bool)> setMediaUploadInProgress;
    std::function<void(const QString&)> setMediaUploadProgressText;
    std::function<void(const QJsonObject&, bool)> applyCurrentUserProfileObject;
    std::function<void(int, const QString&, const QString&, const QString&, const QString&, const QString&, int, bool)>
        applyCurrentUserProfileValues;
    std::function<void(const ProfileAppliedUserState&)> syncCurrentUserProfileState;
    std::function<void()> ensureChatListInitialized;
    std::function<void()> ensureApplyInitialized;
    std::function<void()> bootstrapDialogs;
    std::function<void()> bootstrapGroups;
    std::function<void()> requestRelationBootstrap;
    std::function<void()> refreshContactLoadMoreState;
    std::function<void()> refreshDialogModelIncremental;
    std::function<void()> flushIncomingMessageRouter;
    std::function<void(int)> loadDraftStore;
    std::function<void()> startPendingAttachmentRunner;
    std::function<void(const QVariantList&)> addPendingAttachments;
    std::function<void(const QString&)> removePendingAttachmentById;
    std::function<void()> jumpChatWithCurrentContact;
    std::function<void(int)> selectGroupIndex;
    std::function<void()> syncShellViewModelState;
    std::function<void()> emitCurrentDialogUidChangedIfNeeded;
};

struct AppPortRegistryContext
{
    AppPortRegistryRefs refs;
    AppPortRegistryConstants constants;
    AppPortRegistryQueries queries;
    AppPortRegistryActions actions;
};

class AppPortRegistry
{
public:
    explicit AppPortRegistry(AppPortRegistryContext context);

    void bindSessionPorts();
    void bindMediaPorts();
    void bindCallPorts();
    void bindFeaturePorts();
    void bindAuthFeaturePorts();
    void bindContactFeaturePorts();
    void bindGroupFeaturePorts();
    void bindProfileFeaturePorts();
    void bindConnectionPorts();

    SessionAuthPort makeSessionAuthPort();
    PostLoginBootstrapPort makePostLoginBootstrapPort();
    RelationBootstrapPort makeRelationBootstrapPort();
    RegisterCountdownPort makeRegisterCountdownPort();
    SessionLogoutPort makeSessionLogoutPort();

private:
    int page() const;
    bool contactsReady() const;
    bool isChatTransportReady() const;
    bool busy() const;
    int currentUserUid() const;
    int currentDialogUid() const;
    qint64 currentGroupId() const;
    bool hasCurrentContact() const;
    int currentContactUid() const;
    QString currentContactName() const;
    QString currentContactIcon() const;
    QString currentUserName() const;
    QString currentUserNick() const;
    QString currentUserIcon() const;
    QString currentUserDesc() const;
    QString currentUserId() const;
    int currentGroupRole() const;
    bool currentGroupCanChangeInfo() const;
    bool currentGroupCanManageAdmins() const;
    QVariantList currentPendingAttachments() const;
    QString normalizeIconPath(QString icon) const;

    void setPage(int page);
    void clearTip();
    void switchToLogin();
    void setTip(const QString& tip, bool isError);
    void setBusy(bool value);
    void setSearchPending(bool pending);
    void setSearchStatus(const QString& text, bool isError);
    void setAuthStatus(const QString& text, bool isError);
    void setSettingsStatus(const QString& text, bool isError);
    void setGroupStatus(const QString& text, bool isError);
    void setChatLoadingMore(bool loading);
    void setPrivateHistoryLoading(bool loading);
    void setCanLoadMorePrivateHistory(bool canLoad);
    void setContactLoadingMore(bool loading);
    void setDialogsReady(bool ready);
    void setContactsReady(bool ready);
    void setGroupsReady(bool ready);
    void setApplyReady(bool ready);
    void setContactPane(int pane);
    void setCurrentContact(int uid,
                           const QString& name,
                           const QString& nick,
                           const QString& icon,
                           const QString& back,
                           int sex,
                           const QString& userId = QString());
    void setCurrentChatPeerName(const QString& name);
    void setCurrentChatPeerIcon(const QString& icon);
    void setCurrentDraftText(const QString& text);
    void setCurrentPendingAttachments(const QVariantList& attachments);
    void setCurrentDialogPinned(bool pinned);
    void setCurrentDialogMuted(bool muted);
    void setPendingReplyContext(const QString& msgId, const QString& senderName, const QString& previewText);
    void setMediaUploadInProgress(bool inProgress);
    void setMediaUploadProgressText(const QString& text);
    void applyCurrentUserProfile(const QJsonObject& profile, bool preserveExistingIcon);
    void applyCurrentUserProfile(int uid,
                                 const QString& name,
                                 const QString& nick,
                                 const QString& icon,
                                 const QString& desc,
                                 const QString& userId,
                                 int sex,
                                 bool preserveExistingIcon);
    void ensureChatListInitialized();
    void ensureApplyInitialized();
    void bootstrapDialogs();
    void bootstrapGroups();
    void requestRelationBootstrap();
    void refreshContactLoadMoreState();
    void refreshDialogModelIncremental();
    void flushIncomingMessageRouter();
    void loadDraftStore(int ownerUid);
    void startPendingAttachmentRunner();
    void addPendingAttachments(const QVariantList& attachments);
    void removePendingAttachmentById(const QString& attachmentId);
    void jumpChatWithCurrentContact();
    void selectGroupIndex(int index);
    void syncShellViewModelState();
    void emitCurrentDialogUidChangedIfNeeded();

    AppPortRegistryConstants _constants;
    AppPortRegistryQueries _queries;
    AppPortRegistryActions _actions;
    QObject& _async_receiver;
    AppShellStateController& _shell_state;
    AppMediaUploadRuntimeState& _media_upload_state;
    AppCurrentChatState& _chat_state;
    ClientGateway& _gateway;
    AppFeatureRegistry& _features;
    QTimer& _heartbeat_timer;
    QTimer& _chat_login_timeout_timer;
    std::unique_ptr<AppSessionCoordinator>& _session_coordinator;
    std::unique_ptr<MediaCoordinator>& _media_coordinator;
    std::unique_ptr<CallCoordinator>& _call_coordinator;
    std::unique_ptr<AppChatConnectionCoordinator>& _chat_connection_coordinator;
};
