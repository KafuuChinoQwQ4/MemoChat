#pragma once

#include <QJsonObject>
#include <QString>
#include <QTimer>
#include <QVariantList>
#include <QVector>
#include <functional>
#include <memory>
#include <vector>

#include "AppControllerConnectionState.h"
#include "global.h"

class CallController;
struct FriendInfo;
struct UserInfo;
struct UploadedMediaInfo;

struct RegisterCountdownPort
{
    std::function<int()> countdownSeconds;
    std::function<void(int)> setCountdownSeconds;
    std::function<void(bool)> setRegisterSuccessPage;
    std::function<void()> switchToLogin;
};

struct SessionAuthPort
{
    std::function<bool(const QString&, QString*)> checkEmail;
    std::function<bool(const QString&, QString*)> checkPassword;
    std::function<bool(const QString&, QString*)> checkUser;
    std::function<bool(const QString&, QString*)> checkVerifyCode;
    std::function<bool(const QString&, QJsonObject&)> parseJson;
    std::function<void(const QString&, bool)> setTip;
    std::function<void(bool)> setBusy;
    std::function<void(const QString&, const QString&)> sendLogin;
    std::function<void(const QString&, Modules)> sendVerifyCode;
    std::function<void(const QString&, const QString&, const QString&, const QString&, const QString&)> sendRegister;
    std::function<void(const QString&, const QString&, const QString&, const QString&)> sendResetPassword;
    std::function<bool()> registerCodeRequestPending;
    std::function<int()> registerCodeCooldownSeconds;
    std::function<void(bool)> setRegisterCodeRequestPending;
    std::function<void(int)> setRegisterCodeCooldownSeconds;
    std::function<void(bool)> setRegisterSuccessPage;
    std::function<void(int)> setRegisterCountdown;
    std::function<void()> startRegisterCountdown;
    std::function<void()> prepareLoginAttempt;
    std::function<void(bool)> setIgnoreNextLoginDisconnect;
    std::function<void(const ServerInfo&, const QJsonObject&)> applyLoginSuccess;
};

struct PostLoginBootstrapSnapshot
{
    bool isChatPage = false;
    bool postLoginBootstrapStarted = false;
    bool chatTransportReady = false;
    bool chatLoginCompleted = false;
    bool dialogsReady = false;
    int pendingUid = 0;
    QString pendingToken;
    QString endpointServerName;
    qint64 loginStartedMs = 0;
    qint64 httpLoginFinishedMs = 0;
    qint64 connectStartedMs = 0;
    qint64 connectFinishedMs = 0;
};

struct PostLoginBootstrapPort
{
    std::function<PostLoginBootstrapSnapshot()> snapshot;
    std::function<std::shared_ptr<UserInfo>()> currentUserInfo;
    std::function<void(bool)> setIgnoreNextLoginDisconnect;
    std::function<void()> stopLoginTimeoutTimer;
    std::function<void()> resetReconnectState;
    std::function<void()> resetHeartbeatTracking;
    std::function<void(qint64)> setLastHeartbeatAckMs;
    std::function<void()> switchToChatPage;
    std::function<void()> resetChatEntryRuntime;
    std::function<void(const std::shared_ptr<UserInfo>&, const QString&)> applyLoggedInUserSession;
    std::function<void()> clearMissingUserDialogState;
    std::function<void(bool)> setPostLoginBootstrapStarted;
    std::function<void(bool)> setChatLoginCompleted;
    std::function<void(int, std::function<void()>)> runDelayed;
    std::function<void(int)> openCachesAndDraftsForUser;
    std::function<void()> bootstrapDialogs;
    std::function<void()> ensureContactsInitialized;
    std::function<void()> ensureApplyInitialized;
    std::function<void()> requestRelationBootstrap;
    std::function<void(int)> startHeartbeatTimer;
    std::function<void()> sendHeartbeatNow;
    std::function<void()> ensureChatListInitialized;
};

struct SessionLogoutPort
{
    std::function<int()> currentUserUid;
    std::function<void()> stopSessionTimers;
    std::function<void()> resetPresenceSurfaces;
    std::function<void()> resetConnectionRuntime;
    std::function<void()> closeNetworkResources;
    std::function<void()> resetAuthShellState;
    std::function<void()> resetFeatureModelsForLogout;
    std::function<void()> resetFeatureRuntimeForLogout;
    std::function<void(int)> clearCurrentUserState;
    std::function<void()> resetMediaRuntimeForLogout;
    std::function<void()> clearDownloadAuthContext;
};

struct RelationBootstrapSnapshot
{
    bool isChatPage = false;
    bool chatTransportConnected = false;
    bool contactsReady = false;
    int protocolVersion = 2;
    int pendingUid = 0;
    QString traceId;
    int currentChatPeerUid = 0;
    qint64 currentGroupId = 0;
};

struct RelationBootstrapPort
{
    std::function<RelationBootstrapSnapshot()> snapshot;
    std::function<void(const QByteArray&)> sendRelationBootstrap;
    std::function<void(bool)> setChatListInitialized;
    std::function<void()> ensureChatListInitialized;
    std::function<std::vector<std::shared_ptr<FriendInfo>>()> friendListSnapshot;
    std::function<void(const std::shared_ptr<FriendInfo>&)> upsertChatListFriend;
    std::function<void(const std::vector<std::shared_ptr<FriendInfo>>&)> setContactsFromSnapshot;
    std::function<void()> refreshContactProfiles;
    std::function<void()> refreshContactLoadMoreState;
    std::function<void(bool)> setContactsReady;
    std::function<void()> refreshApplySnapshot;
    std::function<void()> refreshDialogModelIncremental;
    std::function<void()> flushIncomingMessageRouter;
    std::function<std::shared_ptr<FriendInfo>(int)> friendById;
    std::function<void(const QString&)> setCurrentChatPeerName;
    std::function<void(const QString&)> setCurrentChatPeerIcon;
};

struct MediaSendSnapshot
{
    int currentChatUid = 0;
    qint64 currentGroupId = 0;
    int currentDialogUid = 0;
    bool uploadInProgress = false;
    QVariantList pendingAttachments;
    QString replyToMsgId;
    QString replyTargetName;
    QString replyPreviewText;
};

struct MediaSendPort
{
    std::function<MediaSendSnapshot()> snapshot;
    std::function<void(const QString&, bool)> setTip;
    std::function<bool(const QString&, const QString&)> dispatchPrivateContent;
    std::function<bool(const QString&, const QString&)> dispatchGroupContent;
    std::function<void()> cancelReply;
    std::function<void(int, int, qint64, const QVariantList&)> beginPendingAttachmentSend;
    std::function<void()> startPendingAttachmentRunner;
    std::function<void(const QVariantList&)> addPendingAttachments;
    std::function<void(const QString&)> removePendingAttachmentById;
    std::function<void(int)> clearPendingAttachmentsForDialog;
    std::function<void(const QVariantList&)> setCurrentPendingAttachments;
};

struct CallShellSnapshot
{
    bool hasCurrentContact = false;
    qint64 currentGroupId = 0;
    int currentChatUid = 0;
    int currentContactUid = 0;
    QString currentContactName;
    QString currentContactIcon;
    int selfUid = 0;
    QString authToken;
};

struct CallShellPort
{
    std::function<CallShellSnapshot()> snapshot;
    std::function<std::shared_ptr<FriendInfo>(int)> friendById;
    std::function<void(const std::shared_ptr<FriendInfo>&)> setCurrentContactFromFriend;
    std::function<bool(const QString&, QJsonObject&)> parseJson;
    std::function<QString(QString)> normalizeIconPath;
    std::function<void(const QString&, bool)> setAuthStatus;
    std::function<void(int, std::function<void()>)> runDelayed;
    CallController* callController = nullptr;
};

class SessionAuthCoordinator
{
public:
    explicit SessionAuthCoordinator(SessionAuthPort port);

    void login(const QString& email, const QString& password);
    void requestRegisterCode(const QString& email);
    void registerUser(const QString& user,
                      const QString& email,
                      const QString& password,
                      const QString& confirm,
                      const QString& verifyCode);
    void requestResetCode(const QString& email);
    void resetPassword(const QString& user, const QString& email, const QString& password, const QString& verifyCode);
    void onLoginHttpFinished(ReqId id, QString res, ErrorCodes err);
    void onRegisterHttpFinished(ReqId id, QString res, ErrorCodes err);
    void onResetHttpFinished(ReqId id, QString res, ErrorCodes err);
    void onRegisterCodeCooldownTimeout();

private:
    bool checkEmailValid(const QString& email);
    bool checkPasswordValid(const QString& password);
    bool checkUserValid(const QString& user);
    bool checkVerifyCodeValid(const QString& code);
    void clearRegisterCodeRequestCooldown();
    void startRegisterCodeCooldownTimer();
    void stopRegisterCodeCooldownTimer();

    SessionAuthPort _port;
    QTimer _registerCodeCooldownTimer;
};

class SessionRelationBootstrap
{
public:
    explicit SessionRelationBootstrap(RelationBootstrapPort port);

    void requestRelationBootstrap();
    void onRelationBootstrapUpdated();

private:
    RelationBootstrapPort _port;
};

class SessionChatEntryCoordinator
{
public:
    explicit SessionChatEntryCoordinator(PostLoginBootstrapPort port);

    void onSwitchToChat();
    void beginPostLoginBootstrap();
    void runPostLoginBootstrap();

private:
    PostLoginBootstrapPort _port;
};

class RegisterCountdownController
{
public:
    explicit RegisterCountdownController(RegisterCountdownPort port);

    void onRegisterCountdownTimeout();
    void startTimer();
    void stopTimer();

private:
    RegisterCountdownPort _port;
    QTimer _timer;
};

class AppSessionCoordinator
{
public:
    AppSessionCoordinator(SessionAuthPort authPort,
                          PostLoginBootstrapPort chatEntryPort,
                          RelationBootstrapPort relationBootstrapPort,
                          RegisterCountdownPort registerCountdownPort,
                          SessionLogoutPort logoutPort);

    void login(const QString& email, const QString& password);
    void requestRegisterCode(const QString& email);
    void registerUser(const QString& user,
                      const QString& email,
                      const QString& password,
                      const QString& confirm,
                      const QString& verifyCode);
    void requestResetCode(const QString& email);
    void resetPassword(const QString& user, const QString& email, const QString& password, const QString& verifyCode);
    void onLoginHttpFinished(ReqId id, QString res, ErrorCodes err);
    void onRegisterHttpFinished(ReqId id, QString res, ErrorCodes err);
    void onResetHttpFinished(ReqId id, QString res, ErrorCodes err);
    void onSwitchToChat();
    void beginPostLoginBootstrap();
    void runPostLoginBootstrap();
    void requestRelationBootstrap();
    void onRelationBootstrapUpdated();
    void onRegisterCountdownTimeout();
    void onRegisterCodeCooldownTimeout();
    void stopRegisterCountdownTimer();
    void resetForLogout();
    const AppPendingLoginState& pendingLoginState() const;
    const AppChatEndpointState& chatEndpointState() const;
    const AppChatRecoveryState& chatRecoveryState() const;
    QString authToken() const;
    int pendingUid() const;
    void resetLoginAttemptState(qint64 loginStartedMs);
    void applyLoginSuccessState(const ServerInfo& serverInfo, const QString& traceId, qint64 httpLoginFinishedMs);
    void clearSessionForLogout();
    void setIgnoreNextLoginDisconnect(bool ignore);
    void setReconnecting(bool reconnecting);
    void setReconnectAttempts(int attempts);
    void setLoginTcpFallbackAttempted(bool attempted);
    void setConnectStartedMs(qint64 connectStartedMs);
    void setConnectFinishedMs(qint64 connectFinishedMs);
    void setCurrentEndpoint(const ChatEndpoint& endpoint);
    void setEndpointIndex(int endpointIndex);
    void setLastHeartbeatSentMs(qint64 sentMs);
    void setLastHeartbeatAckMs(qint64 ackMs);
    void setHeartbeatAckMissCount(int missCount);
    void setClosedByHeartbeatWatchdog(bool closed);

private:
    AppPendingLoginState _pending_login_state;
    AppChatEndpointState _chat_endpoint_state;
    AppChatRecoveryState _chat_recovery_state;
    std::unique_ptr<SessionAuthCoordinator> _auth;
    std::unique_ptr<SessionChatEntryCoordinator> _chat_entry;
    std::unique_ptr<SessionRelationBootstrap> _relation_bootstrap;
    std::unique_ptr<RegisterCountdownController> _register_countdown;
    SessionLogoutPort _logout_port;
};

class CallCoordinator
{
public:
    explicit CallCoordinator(CallShellPort port);

    void startVoiceChat();
    void startVideoChat();
    void acceptIncomingCall();
    void rejectIncomingCall();
    void endCurrentCall();
    void finalizeEndedCall(const QString& statusText);
    void toggleCallMuted();
    void toggleCallCamera();
    void onCallHttpFinished(ReqId id, QString res, ErrorCodes err);
    void onCallEvent(QJsonObject payload);
    void onLivekitRoomJoined();
    void onLivekitRemoteTrackReady();
    void onLivekitRoomDisconnected(const QString& reason, bool recoverable);
    void onLivekitPermissionError(const QString& deviceType, const QString& message);
    void onLivekitMediaError(const QString& message);
    void onLivekitReconnecting(const QString& message);

private:
    CallShellSnapshot snapshot() const;
    CallController* callController() const;
    bool ensureCallTargetFromCurrentChat();
    void startCallFlow(const QString& callType);

    CallShellPort _port;
};

class MediaCoordinator
{
public:
    explicit MediaCoordinator(MediaSendPort port);

    void sendTextMessage(const QString& text);
    void sendCurrentComposerPayload(const QString& text);
    void sendImageMessage();
    void sendFileMessage();
    void removePendingAttachment(const QString& attachmentId);
    void clearPendingAttachments();

private:
    bool dispatchTextMessage(const QString& text);

    MediaSendPort _port;
};
