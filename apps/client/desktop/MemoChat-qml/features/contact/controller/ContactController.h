#ifndef CONTACTCONTROLLER_H
#define CONTACTCONTROLLER_H

#include <QObject>
#include <QHash>
#include <QVariantList>
#include <QVariantMap>
#include <QSet>
#include <QString>
#include <functional>
#include <memory>
#include <vector>

#include "global.h"

class AddFriendApply;
class ApplyRequestModel;
class ApplyInfo;
class AuthInfo;
class AuthRsp;
class ClientGateway;
class FriendListModel;
class FriendInfo;
class SearchInfo;
class SearchResultModel;

struct ContactCommandPort
{
    std::function<void()> openCurrentContactChat;
    std::function<void()> requestRelationBootstrap;
};

struct ContactBootstrapPort
{
    std::function<void()> ensureChatListInitialized;
    std::function<std::vector<std::shared_ptr<FriendInfo>>()> nextPage;
    std::function<void()> markPageLoaded;
    std::function<bool()> loadFinished;
};

struct ContactApplyBootstrapPort
{
    std::function<std::vector<std::shared_ptr<ApplyInfo>>()> applySnapshot;
};

class ContactController : public QObject
{
    Q_OBJECT
    Q_MOC_INCLUDE("ApplyRequestModel.h")
    Q_MOC_INCLUDE("FriendListModel.h")
    Q_MOC_INCLUDE("SearchResultModel.h")
    Q_PROPERTY(int contactPane READ contactPane NOTIFY contactPaneChanged)
    Q_PROPERTY(QString currentContactName READ currentContactName NOTIFY currentContactChanged)
    Q_PROPERTY(QString currentContactNick READ currentContactNick NOTIFY currentContactChanged)
    Q_PROPERTY(QString currentContactIcon READ currentContactIcon NOTIFY currentContactChanged)
    Q_PROPERTY(QString currentContactBack READ currentContactBack NOTIFY currentContactChanged)
    Q_PROPERTY(int currentContactSex READ currentContactSex NOTIFY currentContactChanged)
    Q_PROPERTY(QString currentContactUserId READ currentContactUserId NOTIFY currentContactChanged)
    Q_PROPERTY(int currentContactUid READ currentContactUid NOTIFY currentContactChanged)
    Q_PROPERTY(bool hasCurrentContact READ hasCurrentContact NOTIFY currentContactChanged)
    Q_PROPERTY(FriendListModel* contactListModel READ contactListModel NOTIFY modelChanged)
    Q_PROPERTY(SearchResultModel* searchResultModel READ searchResultModel NOTIFY modelChanged)
    Q_PROPERTY(ApplyRequestModel* applyRequestModel READ applyRequestModel NOTIFY modelChanged)
    Q_PROPERTY(bool searchPending READ searchPending NOTIFY searchPendingChanged)
    Q_PROPERTY(QString searchStatusText READ searchStatusText NOTIFY searchStatusChanged)
    Q_PROPERTY(bool searchStatusError READ searchStatusError NOTIFY searchStatusChanged)
    Q_PROPERTY(QString authStatusText READ authStatusText NOTIFY authStatusChanged)
    Q_PROPERTY(bool authStatusError READ authStatusError NOTIFY authStatusChanged)
    Q_PROPERTY(bool hasPendingApply READ hasPendingApply NOTIFY pendingApplyChanged)
    Q_PROPERTY(bool contactLoadingMore READ contactLoadingMore NOTIFY contactLoadingMoreChanged)
    Q_PROPERTY(bool canLoadMoreContacts READ canLoadMoreContacts NOTIFY canLoadMoreContactsChanged)
    Q_PROPERTY(bool contactsReady READ contactsReady NOTIFY contactsReadyChanged)
    Q_PROPERTY(bool applyReady READ applyReady NOTIFY applyReadyChanged)

public:
    explicit ContactController(ClientGateway* gateway, QObject* parent = nullptr);

    int contactPane() const;
    QString currentContactName() const;
    QString currentContactNick() const;
    QString currentContactIcon() const;
    QString currentContactBack() const;
    int currentContactSex() const;
    QString currentContactUserId() const;
    int currentContactUid() const;
    bool hasCurrentContact() const;
    FriendListModel* contactListModel() const;
    SearchResultModel* searchResultModel() const;
    ApplyRequestModel* applyRequestModel() const;
    bool searchPending() const;
    QString searchStatusText() const;
    bool searchStatusError() const;
    QString authStatusText() const;
    bool authStatusError() const;
    bool hasPendingApply() const;
    bool contactLoadingMore() const;
    bool canLoadMoreContacts() const;
    bool contactsReady() const;
    bool applyReady() const;

    Q_INVOKABLE void ensureContactsInitialized();
    Q_INVOKABLE void ensureApplyInitialized();
    Q_INVOKABLE void selectContactIndex(int index);
    Q_INVOKABLE void searchUser(const QString& uidText);
    Q_INVOKABLE void clearSearchState();
    Q_INVOKABLE void requestAddFriend(int uid, const QString& bakName, const QVariantList& labels = QVariantList());
    Q_INVOKABLE void approveFriend(int uid, const QString& backName, const QVariantList& labels = QVariantList());
    Q_INVOKABLE QVariantMap contactProfileByUid(int uid) const;
    Q_INVOKABLE void refreshContactProfileByUid(int uid);
    Q_INVOKABLE void deleteFriend(int uid);
    Q_INVOKABLE void showApplyRequests();
    Q_INVOKABLE void jumpChatWithCurrentContact();
    Q_INVOKABLE void loadMoreContacts();
    Q_INVOKABLE void clearAuthStatus();

    bool sendSearchUser(const QString& uidText, QString* errorText) const;
    void sendAddFriend(int selfUid,
                       const QString& selfName,
                       int targetUid,
                       const QString& bakName,
                       const QVariantList& labels) const;
    void sendApproveFriend(int selfUid, int targetUid, const QString& remark, const QVariantList& labels) const;
    void sendDeleteFriend(int selfUid, int friendUid) const;
    void handleContactHttpFinished(ReqId id, const QString& res, ErrorCodes err);
    void setCommandPort(ContactCommandPort port);
    void setBootstrapPort(ContactBootstrapPort port);
    void setApplyBootstrapPort(ContactApplyBootstrapPort port);
    void refreshApplySnapshot();

    void resetContactFeature();
    void setContactPane(int pane);
    void clearCurrentContact();
    void setCurrentContact(int uid,
                           const QString& name,
                           const QString& nick,
                           const QString& icon,
                           const QString& back,
                           int sex,
                           const QString& userId);
    void setContacts(const std::vector<std::shared_ptr<FriendInfo>>& contacts);
    void appendContacts(const std::vector<std::shared_ptr<FriendInfo>>& contacts);
    void upsertContact(const std::shared_ptr<FriendInfo>& friendInfo);
    void upsertContact(const std::shared_ptr<AuthInfo>& authInfo);
    void upsertContact(const std::shared_ptr<AuthRsp>& authRsp);
    void refreshContactProfiles();
    void removeContactByUid(int uid);
    void setApplies(const std::vector<std::shared_ptr<ApplyInfo>>& applies);
    void upsertApply(const std::shared_ptr<ApplyInfo>& apply);
    void upsertApply(const std::shared_ptr<AddFriendApply>& apply);
    void markApplyApproved(int uid);
    void setApplyPending(int uid, bool pending);
    void clearSearchResultOnly();
    void setSearchResult(const std::shared_ptr<SearchInfo>& searchInfo);
    void setSearchPending(bool pending);
    void setSearchStatus(const QString& text, bool isError);
    void setAuthStatus(const QString& text, bool isError);
    void setContactLoadingMore(bool loading);
    void setCanLoadMoreContacts(bool canLoad);
    void setContactsReady(bool ready);
    void setApplyReady(bool ready);

    void syncModels(FriendListModel* contactListModel,
                    SearchResultModel* searchResultModel,
                    ApplyRequestModel* applyRequestModel);
    void syncContactPane(int pane);
    void syncCurrentContact(int uid,
                            const QString& name,
                            const QString& nick,
                            const QString& icon,
                            const QString& back,
                            int sex,
                            const QString& userId);
    void syncSearchPending(bool pending);
    void syncSearchStatus(const QString& text, bool isError);
    void syncAuthStatus(const QString& text, bool isError);
    void syncHasPendingApply(bool hasPending);
    void syncContactLoadingMore(bool loading);
    void syncCanLoadMoreContacts(bool canLoad);
    void syncContactsReady(bool ready);
    void syncApplyReady(bool ready);

signals:
    void contactPaneChanged();
    void currentContactChanged();
    void modelChanged();
    void searchPendingChanged();
    void searchStatusChanged();
    void authStatusChanged();
    void pendingApplyChanged();
    void contactLoadingMoreChanged();
    void canLoadMoreContactsChanged();
    void contactsReadyChanged();
    void applyReadyChanged();
    void contactProfilesChanged();
    void currentContactSelected(int uid);

private:
    ClientGateway* _gateway;
    ContactCommandPort _command_port;
    ContactBootstrapPort _bootstrap_port;
    ContactApplyBootstrapPort _apply_bootstrap_port;
    FriendListModel* _contact_list_model = nullptr;
    SearchResultModel* _search_result_model = nullptr;
    ApplyRequestModel* _apply_request_model = nullptr;
    int _contact_pane = 0;
    QString _current_contact_name;
    QString _current_contact_nick;
    QString _current_contact_icon = QStringLiteral("qrc:/res/head_1.jpg");
    QString _current_contact_back;
    int _current_contact_sex = 0;
    QString _current_contact_user_id;
    int _current_contact_uid = 0;
    bool _search_pending = false;
    QString _search_status_text;
    bool _search_status_error = false;
    QString _auth_status_text;
    bool _auth_status_error = false;
    bool _has_pending_apply = false;
    bool _contact_loading_more = false;
    bool _can_load_more_contacts = false;
    bool _contacts_ready = false;
    bool _apply_ready = false;
    QHash<int, QVariantMap> _profile_lookup_cache;
    QSet<int> _profile_lookup_pending_uids;

    void setHasPendingApply(bool hasPending);
    void refreshContactLoadMoreState();
    void refreshCurrentContactFromStore();
    void requestPublicProfileByUid(int uid);
};

#endif // CONTACTCONTROLLER_H
