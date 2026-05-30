#pragma once

#include <QString>
#include <QVariantList>
#include <memory>

#include "global.h"

class AppController;
struct UploadedMediaInfo;

class SessionAuthCoordinator
{
public:
    explicit SessionAuthCoordinator(AppController& controller);

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

private:
    AppController& _app;
};

class SessionRelationBootstrap
{
public:
    explicit SessionRelationBootstrap(AppController& controller);

    void requestRelationBootstrap();
    void onRelationBootstrapUpdated();

private:
    AppController& _app;
};

class SessionChatEntryCoordinator
{
public:
    explicit SessionChatEntryCoordinator(AppController& controller);

    void onSwitchToChat();
    void beginPostLoginBootstrap();
    void runPostLoginBootstrap();

private:
    AppController& _app;
};

class RegisterCountdownController
{
public:
    explicit RegisterCountdownController(AppController& controller);

    void onRegisterCountdownTimeout();

private:
    AppController& _app;
};

class AppSessionCoordinator
{
public:
    explicit AppSessionCoordinator(AppController& controller);

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

private:
    AppController& _app;
    std::unique_ptr<SessionAuthCoordinator> _auth;
    std::unique_ptr<SessionChatEntryCoordinator> _chat_entry;
    std::unique_ptr<SessionRelationBootstrap> _relation_bootstrap;
    std::unique_ptr<RegisterCountdownController> _register_countdown;
};

class ContactCoordinatorShell
{
public:
    explicit ContactCoordinatorShell(AppController& controller);

    void searchUser(const QString& uidText);
    void requestAddFriend(int uid, const QString& bakName, const QVariantList& labels);
    void approveFriend(int uid, const QString& backName, const QVariantList& labels);

private:
    AppController& _app;
};

class CallCoordinator
{
public:
    explicit CallCoordinator(AppController& controller);

    void startVoiceChat();
    void startVideoChat();
    void acceptIncomingCall();
    void rejectIncomingCall();
    void endCurrentCall();
    void toggleCallMuted();
    void toggleCallCamera();

private:
    bool ensureCallTargetFromCurrentChat();
    void startCallFlow(const QString& callType);

    AppController& _app;
};

class ProfileCoordinator
{
public:
    explicit ProfileCoordinator(AppController& controller);

    void chooseAvatar(int source = 0);
    void saveProfile(const QString& nick, const QString& desc);

private:
    AppController& _app;
};

class GroupCoordinator
{
public:
    explicit GroupCoordinator(AppController& controller);

    void createGroup(const QString& name, const QVariantList& memberUserIdList);
    void inviteGroupMember(const QString& userId, const QString& reason);
    void applyJoinGroup(const QString& groupCode, const QString& reason);
    void reviewGroupApply(qint64 applyId, bool agree);
    void sendGroupTextMessage(const QString& text);
    void sendGroupImageMessage();
    void sendGroupFileMessage();
    void updateGroupAnnouncement(const QString& announcement);
    void setGroupAdmin(const QString& userId, bool isAdmin, qint64 permissionBits);
    void muteGroupMember(const QString& userId, int muteSeconds);
    void kickGroupMember(const QString& userId);
    void quitCurrentGroup();
    void dissolveCurrentGroup();

private:
    AppController& _app;
};

class MediaCoordinator
{
public:
    explicit MediaCoordinator(AppController& controller);

    void sendTextMessage(const QString& text);
    void sendCurrentComposerPayload(const QString& text);
    void sendImageMessage();
    void sendFileMessage();
    void removePendingAttachment(const QString& attachmentId);
    void clearPendingAttachments();

private:
    AppController& _app;
};
