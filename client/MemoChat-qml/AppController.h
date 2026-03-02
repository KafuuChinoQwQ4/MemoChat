#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H

#include <QObject>
#include <QTimer>
#include <QJsonObject>
#include <QVariantList>
#include <QMap>
#include "global.h"
#include "AuthController.h"
#include "ChatController.h"
#include "ClientGateway.h"
#include "ContactController.h"
#include "FriendListModel.h"
#include "ChatMessageModel.h"
#include "SearchResultModel.h"
#include "ApplyRequestModel.h"
#include "ProfileController.h"

class AppController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Page page READ page NOTIFY pageChanged)
    Q_PROPERTY(QString tipText READ tipText NOTIFY tipChanged)
    Q_PROPERTY(bool tipError READ tipError NOTIFY tipChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(bool registerSuccessPage READ registerSuccessPage NOTIFY registerSuccessPageChanged)
    Q_PROPERTY(int registerCountdown READ registerCountdown NOTIFY registerCountdownChanged)
    Q_PROPERTY(ChatTab chatTab READ chatTab NOTIFY chatTabChanged)
    Q_PROPERTY(QString currentUserName READ currentUserName NOTIFY currentUserChanged)
    Q_PROPERTY(QString currentUserNick READ currentUserNick NOTIFY currentUserChanged)
    Q_PROPERTY(QString currentUserIcon READ currentUserIcon NOTIFY currentUserChanged)
    Q_PROPERTY(QString currentUserDesc READ currentUserDesc NOTIFY currentUserChanged)
    Q_PROPERTY(ContactPane contactPane READ contactPane NOTIFY contactPaneChanged)
    Q_PROPERTY(QString currentContactName READ currentContactName NOTIFY currentContactChanged)
    Q_PROPERTY(QString currentContactNick READ currentContactNick NOTIFY currentContactChanged)
    Q_PROPERTY(QString currentContactIcon READ currentContactIcon NOTIFY currentContactChanged)
    Q_PROPERTY(QString currentContactBack READ currentContactBack NOTIFY currentContactChanged)
    Q_PROPERTY(int currentContactSex READ currentContactSex NOTIFY currentContactChanged)
    Q_PROPERTY(bool hasCurrentContact READ hasCurrentContact NOTIFY currentContactChanged)
    Q_PROPERTY(QString currentChatPeerName READ currentChatPeerName NOTIFY currentChatPeerChanged)
    Q_PROPERTY(QString currentChatPeerIcon READ currentChatPeerIcon NOTIFY currentChatPeerChanged)
    Q_PROPERTY(bool hasCurrentChat READ hasCurrentChat NOTIFY currentChatPeerChanged)
    Q_PROPERTY(bool hasCurrentGroup READ hasCurrentGroup NOTIFY currentGroupChanged)
    Q_PROPERTY(QString currentGroupName READ currentGroupName NOTIFY currentGroupChanged)
    Q_PROPERTY(FriendListModel* chatListModel READ chatListModel CONSTANT)
    Q_PROPERTY(FriendListModel* groupListModel READ groupListModel CONSTANT)
    Q_PROPERTY(FriendListModel* contactListModel READ contactListModel CONSTANT)
    Q_PROPERTY(ChatMessageModel* messageModel READ messageModel CONSTANT)
    Q_PROPERTY(SearchResultModel* searchResultModel READ searchResultModel CONSTANT)
    Q_PROPERTY(ApplyRequestModel* applyRequestModel READ applyRequestModel CONSTANT)
    Q_PROPERTY(bool searchPending READ searchPending NOTIFY searchPendingChanged)
    Q_PROPERTY(QString searchStatusText READ searchStatusText NOTIFY searchStatusChanged)
    Q_PROPERTY(bool searchStatusError READ searchStatusError NOTIFY searchStatusChanged)
    Q_PROPERTY(bool hasPendingApply READ hasPendingApply NOTIFY pendingApplyChanged)
    Q_PROPERTY(bool chatLoadingMore READ chatLoadingMore NOTIFY chatLoadingMoreChanged)
    Q_PROPERTY(bool canLoadMoreChats READ canLoadMoreChats NOTIFY canLoadMoreChatsChanged)
    Q_PROPERTY(bool contactLoadingMore READ contactLoadingMore NOTIFY contactLoadingMoreChanged)
    Q_PROPERTY(bool canLoadMoreContacts READ canLoadMoreContacts NOTIFY canLoadMoreContactsChanged)
    Q_PROPERTY(QString authStatusText READ authStatusText NOTIFY authStatusChanged)
    Q_PROPERTY(bool authStatusError READ authStatusError NOTIFY authStatusChanged)
    Q_PROPERTY(QString settingsStatusText READ settingsStatusText NOTIFY settingsStatusChanged)
    Q_PROPERTY(bool settingsStatusError READ settingsStatusError NOTIFY settingsStatusChanged)
    Q_PROPERTY(QString groupStatusText READ groupStatusText NOTIFY groupStatusChanged)
    Q_PROPERTY(bool groupStatusError READ groupStatusError NOTIFY groupStatusChanged)

public:
    enum Page {
        LoginPage = 0,
        RegisterPage = 1,
        ResetPage = 2,
        ChatPage = 3
    };
    Q_ENUM(Page)

    enum ChatTab {
        ChatTabPage = 0,
        ContactTabPage = 1,
        SettingsTabPage = 2
    };
    Q_ENUM(ChatTab)

    enum ContactPane {
        ApplyRequestPane = 0,
        FriendInfoPane = 1
    };
    Q_ENUM(ContactPane)

    explicit AppController(QObject *parent = nullptr);

    Page page() const;
    QString tipText() const;
    bool tipError() const;
    bool busy() const;
    bool registerSuccessPage() const;
    int registerCountdown() const;
    ChatTab chatTab() const;
    ContactPane contactPane() const;
    QString currentUserName() const;
    QString currentUserNick() const;
    QString currentUserIcon() const;
    QString currentUserDesc() const;
    QString currentContactName() const;
    QString currentContactNick() const;
    QString currentContactIcon() const;
    QString currentContactBack() const;
    int currentContactSex() const;
    bool hasCurrentContact() const;
    QString currentChatPeerName() const;
    QString currentChatPeerIcon() const;
    bool hasCurrentChat() const;
    bool hasCurrentGroup() const;
    QString currentGroupName() const;
    FriendListModel *chatListModel();
    FriendListModel *groupListModel();
    FriendListModel *contactListModel();
    ChatMessageModel *messageModel();
    SearchResultModel *searchResultModel();
    ApplyRequestModel *applyRequestModel();
    bool searchPending() const;
    QString searchStatusText() const;
    bool searchStatusError() const;
    bool hasPendingApply() const;
    bool chatLoadingMore() const;
    bool canLoadMoreChats() const;
    bool contactLoadingMore() const;
    bool canLoadMoreContacts() const;
    QString authStatusText() const;
    bool authStatusError() const;
    QString settingsStatusText() const;
    bool settingsStatusError() const;
    QString groupStatusText() const;
    bool groupStatusError() const;

    Q_INVOKABLE void switchToLogin();
    Q_INVOKABLE void switchToRegister();
    Q_INVOKABLE void switchToReset();
    Q_INVOKABLE void switchChatTab(int tab);
    Q_INVOKABLE void clearTip();
    Q_INVOKABLE void selectChatIndex(int index);
    Q_INVOKABLE void selectGroupIndex(int index);
    Q_INVOKABLE void selectContactIndex(int index);
    Q_INVOKABLE void showApplyRequests();
    Q_INVOKABLE void jumpChatWithCurrentContact();
    Q_INVOKABLE void loadMoreChats();
    Q_INVOKABLE void loadMoreContacts();
    Q_INVOKABLE void sendTextMessage(const QString &text);
    Q_INVOKABLE void sendImageMessage();
    Q_INVOKABLE void sendFileMessage();
    Q_INVOKABLE void openExternalResource(const QString &url);
    Q_INVOKABLE void searchUser(const QString &uidText);
    Q_INVOKABLE void clearSearchState();
    Q_INVOKABLE void requestAddFriend(int uid, const QString &bakName, const QVariantList &labels = QVariantList());
    Q_INVOKABLE void approveFriend(int uid, const QString &backName, const QVariantList &labels = QVariantList());
    Q_INVOKABLE void clearAuthStatus();
    Q_INVOKABLE void startVoiceChat();
    Q_INVOKABLE void startVideoChat();
    Q_INVOKABLE void chooseAvatar();
    Q_INVOKABLE void saveProfile(const QString &nick, const QString &desc);
    Q_INVOKABLE void clearSettingsStatus();
    Q_INVOKABLE void refreshGroupList();
    Q_INVOKABLE void createGroup(const QString &name, const QVariantList &memberUidList = QVariantList());
    Q_INVOKABLE void inviteGroupMember(int uid, const QString &reason = QString());
    Q_INVOKABLE void applyJoinGroup(qint64 groupId, const QString &reason = QString());
    Q_INVOKABLE void reviewGroupApply(qint64 applyId, bool agree);
    Q_INVOKABLE void sendGroupTextMessage(const QString &text);
    Q_INVOKABLE void sendGroupImageMessage();
    Q_INVOKABLE void sendGroupFileMessage();
    Q_INVOKABLE void loadGroupHistory();
    Q_INVOKABLE void updateGroupAnnouncement(const QString &announcement);
    Q_INVOKABLE void setGroupAdmin(int uid, bool isAdmin);
    Q_INVOKABLE void muteGroupMember(int uid, int muteSeconds);
    Q_INVOKABLE void kickGroupMember(int uid);
    Q_INVOKABLE void quitCurrentGroup();
    Q_INVOKABLE void clearGroupStatus();

    Q_INVOKABLE void login(const QString &email, const QString &password);
    Q_INVOKABLE void requestRegisterCode(const QString &email);
    Q_INVOKABLE void registerUser(const QString &user, const QString &email, const QString &password,
                                  const QString &confirm, const QString &verifyCode);
    Q_INVOKABLE void requestResetCode(const QString &email);
    Q_INVOKABLE void resetPassword(const QString &user, const QString &email, const QString &password,
                                   const QString &verifyCode);

signals:
    void pageChanged();
    void tipChanged();
    void busyChanged();
    void registerSuccessPageChanged();
    void registerCountdownChanged();
    void chatTabChanged();
    void contactPaneChanged();
    void currentContactChanged();
    void currentUserChanged();
    void currentChatPeerChanged();
    void searchPendingChanged();
    void searchStatusChanged();
    void pendingApplyChanged();
    void chatLoadingMoreChanged();
    void canLoadMoreChatsChanged();
    void contactLoadingMoreChanged();
    void canLoadMoreContactsChanged();
    void authStatusChanged();
    void settingsStatusChanged();
    void currentGroupChanged();
    void groupStatusChanged();

private slots:
    void onLoginHttpFinished(ReqId id, QString res, ErrorCodes err);
    void onRegisterHttpFinished(ReqId id, QString res, ErrorCodes err);
    void onResetHttpFinished(ReqId id, QString res, ErrorCodes err);
    void onSettingsHttpFinished(ReqId id, QString res, ErrorCodes err);
    void onTcpConnectFinished(bool success);
    void onChatLoginFailed(int err);
    void onSwitchToChat();
    void onRegisterCountdownTimeout();
    void onHeartbeatTimeout();
    void onAddAuthFriend(std::shared_ptr<AuthInfo> authInfo);
    void onAuthRsp(std::shared_ptr<AuthRsp> authRsp);
    void onTextChatMsg(std::shared_ptr<TextChatMsg> msg);
    void onUserSearch(std::shared_ptr<SearchInfo> searchInfo);
    void onFriendApply(std::shared_ptr<AddFriendApply> applyInfo);
    void onNotifyOffline();
    void onConnectionClosed();
    void onGroupListUpdated();
    void onGroupInvite(qint64 groupId, QString groupName, int operatorUid);
    void onGroupApply(qint64 groupId, int applicantUid, QString reason);
    void onGroupMemberChanged(QJsonObject payload);
    void onGroupChatMsg(std::shared_ptr<GroupChatMsg> msg);
    void onGroupRsp(ReqId reqId, int error, QJsonObject payload);

private:
    bool parseJson(const QString &res, QJsonObject &obj);
    bool checkEmailValid(const QString &email);
    bool checkPwdValid(const QString &password);
    bool checkUserValid(const QString &user);
    bool checkVerifyCodeValid(const QString &code);
    bool dispatchChatContent(const QString &content, const QString &previewText);
    bool dispatchGroupChatContent(const QString &content, const QString &previewText);
    void sendCallInvite(const QString &callType);
    QString buildCallJoinUrl(const QString &callType) const;
    void setTip(const QString &tip, bool isError);
    void setBusy(bool value);
    void setPage(Page newPage);
    QString normalizeIconPath(QString icon) const;
    void refreshFriendModels();
    void refreshApplyModel();
    void refreshGroupModel();
    void refreshChatLoadMoreState();
    void refreshContactLoadMoreState();
    void loadCurrentChatMessages();
    void setContactPane(ContactPane pane);
    void setCurrentContact(int uid, const QString &name, const QString &nick, const QString &icon,
                           const QString &back, int sex);
    void setCurrentChatPeerName(const QString &name);
    void setCurrentChatPeerIcon(const QString &icon);
    void selectChatByUid(int uid);
    void setSearchPending(bool pending);
    void setSearchStatus(const QString &text, bool isError);
    void clearSearchResultOnly();
    void setChatLoadingMore(bool loading);
    void setContactLoadingMore(bool loading);
    void setAuthStatus(const QString &text, bool isError);
    void setSettingsStatus(const QString &text, bool isError);
    void setCurrentGroup(qint64 groupId, const QString &name);
    void setGroupStatus(const QString &text, bool isError);

    Page _page;
    QString _tip_text;
    bool _tip_error;
    bool _busy;
    bool _register_success_page;
    int _register_countdown;
    ChatTab _chat_tab;
    ContactPane _contact_pane;
    int _pending_uid;
    QString _pending_token;

    QString _current_user_name;
    QString _current_user_nick;
    QString _current_user_icon;
    QString _current_user_desc;
    int _current_contact_uid;
    QString _current_contact_name;
    QString _current_contact_nick;
    QString _current_contact_icon;
    QString _current_contact_back;
    int _current_contact_sex;
    QString _current_chat_peer_name;
    QString _current_chat_peer_icon;
    int _current_chat_uid;
    qint64 _current_group_id;
    QString _current_group_name;

    FriendListModel _chat_list_model;
    FriendListModel _group_list_model;
    FriendListModel _contact_list_model;
    ChatMessageModel _message_model;
    SearchResultModel _search_result_model;
    ApplyRequestModel _apply_request_model;
    bool _search_pending;
    QString _search_status_text;
    bool _search_status_error;
    bool _chat_loading_more;
    bool _can_load_more_chats;
    bool _contact_loading_more;
    bool _can_load_more_contacts;
    QString _auth_status_text;
    bool _auth_status_error;
    QString _settings_status_text;
    bool _settings_status_error;
    QString _group_status_text;
    bool _group_status_error;
    QMap<int, qint64> _group_uid_map;

    ClientGateway _gateway;
    AuthController _auth_controller;
    ChatController _chat_controller;
    ContactController _contact_controller;
    ProfileController _profile_controller;
    QTimer _register_countdown_timer;
    QTimer _heartbeat_timer;
};

#endif // APPCONTROLLER_H
