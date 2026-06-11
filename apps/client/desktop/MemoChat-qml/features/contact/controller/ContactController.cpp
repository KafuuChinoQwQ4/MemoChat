#include "ContactController.h"
#include "ApplyRequestModel.h"
#include "ClientGateway.h"
#include "ContactRequestPayloads.h"
#include "FriendListModel.h"
#include "IChatTransport.h"
#include "IconPathUtils.h"
#include "SearchResultModel.h"
#include "global.h"
#include "usermgr.h"
#include <QJsonDocument>

#include <utility>

ContactController::ContactController(ClientGateway* gateway, QObject* parent)
    : QObject(parent)
    , _gateway(gateway)
    , _contact_list_model(new FriendListModel(this))
    , _search_result_model(new SearchResultModel(this))
    , _apply_request_model(new ApplyRequestModel(this))
{
    connect(_apply_request_model,
            &ApplyRequestModel::unapprovedCountChanged,
            this,
            [this]()
            {
                setHasPendingApply(_apply_request_model->hasUnapproved());
            });
}

int ContactController::contactPane() const
{
    return _contact_pane;
}

QString ContactController::currentContactName() const
{
    return _current_contact_name;
}

QString ContactController::currentContactNick() const
{
    return _current_contact_nick;
}

QString ContactController::currentContactIcon() const
{
    return _current_contact_icon;
}

QString ContactController::currentContactBack() const
{
    return _current_contact_back;
}

int ContactController::currentContactSex() const
{
    return _current_contact_sex;
}

QString ContactController::currentContactUserId() const
{
    return _current_contact_user_id;
}

int ContactController::currentContactUid() const
{
    return _current_contact_uid;
}

bool ContactController::hasCurrentContact() const
{
    return _current_contact_uid > 0;
}

FriendListModel* ContactController::contactListModel() const
{
    return _contact_list_model;
}

SearchResultModel* ContactController::searchResultModel() const
{
    return _search_result_model;
}

ApplyRequestModel* ContactController::applyRequestModel() const
{
    return _apply_request_model;
}

bool ContactController::searchPending() const
{
    return _search_pending;
}

QString ContactController::searchStatusText() const
{
    return _search_status_text;
}

bool ContactController::searchStatusError() const
{
    return _search_status_error;
}

QString ContactController::authStatusText() const
{
    return _auth_status_text;
}

bool ContactController::authStatusError() const
{
    return _auth_status_error;
}

bool ContactController::hasPendingApply() const
{
    return _has_pending_apply;
}

bool ContactController::contactLoadingMore() const
{
    return _contact_loading_more;
}

bool ContactController::canLoadMoreContacts() const
{
    return _can_load_more_contacts;
}

bool ContactController::contactsReady() const
{
    return _contacts_ready;
}

bool ContactController::applyReady() const
{
    return _apply_ready;
}

void ContactController::ensureContactsInitialized()
{
    if (_contacts_ready)
    {
        return;
    }

    if (_bootstrap_port.ensureChatListInitialized)
    {
        _bootstrap_port.ensureChatListInitialized();
    }
    const std::vector<std::shared_ptr<FriendInfo>> contacts =
        _bootstrap_port.nextPage ? _bootstrap_port.nextPage() : std::vector<std::shared_ptr<FriendInfo>>{};
    setContacts(contacts);
    if (_bootstrap_port.markPageLoaded)
    {
        _bootstrap_port.markPageLoaded();
    }
    if (_bootstrap_port.loadFinished)
    {
        setCanLoadMoreContacts(!_bootstrap_port.loadFinished());
    }
    setContactsReady(true);
}

void ContactController::ensureApplyInitialized()
{
    if (_apply_ready)
    {
        return;
    }
    refreshApplySnapshot();
}

void ContactController::refreshApplySnapshot()
{
    const std::vector<std::shared_ptr<ApplyInfo>> applies = _apply_bootstrap_port.applySnapshot
                                                                ? _apply_bootstrap_port.applySnapshot()
                                                                : std::vector<std::shared_ptr<ApplyInfo>>{};
    setApplies(applies);
    setApplyReady(true);
}

void ContactController::selectContactIndex(int index)
{
    ensureContactsInitialized();
    const QVariantMap contact = _contact_list_model ? _contact_list_model->get(index) : QVariantMap();
    const int uid = contact.value(QStringLiteral("uid")).toInt();
    if (uid > 0)
    {
        const QString name = contact.value(QStringLiteral("name")).toString();
        const QString nick = contact.value(QStringLiteral("nick")).toString();
        const QString icon = contact.value(QStringLiteral("icon")).toString();
        const QString back = contact.value(QStringLiteral("back")).toString();
        const int sex = contact.value(QStringLiteral("sex")).toInt();
        const QString userId = contact.value(QStringLiteral("userId")).toString();
        setCurrentContact(uid, name, nick, icon, back, sex, userId);
        setContactPane(1);
        emit currentContactSelected(uid);
    }
}

void ContactController::searchUser(const QString& uidText)
{
    QString errorText;
    if (!sendSearchUser(uidText, &errorText))
    {
        clearSearchResultOnly();
        setSearchStatus(errorText, true);
        return;
    }

    clearSearchResultOnly();
    setSearchPending(true);
    setSearchStatus("搜索中...", false);
}

void ContactController::clearSearchState()
{
    clearSearchResultOnly();
    setSearchPending(false);
    setSearchStatus(QString(), false);
}

void ContactController::requestAddFriend(int uid, const QString& bakName, const QVariantList& labels)
{
    if (!_gateway || !_gateway->userMgr())
    {
        setSearchStatus("用户状态异常，请重新登录", true);
        return;
    }

    const auto selfInfo = _gateway->userMgr()->GetUserInfo();
    if (!selfInfo)
    {
        setSearchStatus("用户状态异常，请重新登录", true);
        return;
    }
    if (uid == selfInfo->_uid)
    {
        setSearchStatus("不能添加自己", true);
        return;
    }
    if (_gateway->userMgr()->CheckFriendById(uid))
    {
        setSearchStatus("已是好友，无需重复申请", false);
        return;
    }

    sendAddFriend(selfInfo->_uid, selfInfo->_name, uid, bakName, labels);
    setSearchStatus("好友申请已发送", false);
}

void ContactController::approveFriend(int uid, const QString& backName, const QVariantList& labels)
{
    if (uid <= 0)
    {
        setAuthStatus("非法好友申请", true);
        return;
    }
    if (!_gateway || !_gateway->userMgr())
    {
        setAuthStatus("用户状态异常，请重新登录", true);
        return;
    }

    const auto selfInfo = _gateway->userMgr()->GetUserInfo();
    if (!selfInfo)
    {
        setAuthStatus("用户状态异常，请重新登录", true);
        return;
    }
    if (_gateway->userMgr()->CheckFriendById(uid))
    {
        markApplyApproved(uid);
        setAuthStatus("已是好友", false);
        return;
    }

    QString remark = backName.trimmed();
    if (remark.isEmpty())
    {
        remark = _apply_request_model->nameByUid(uid);
    }
    if (remark.isEmpty())
    {
        remark = "MemoChat好友";
    }

    sendApproveFriend(selfInfo->_uid, uid, remark, labels);
    setApplyPending(uid, true);
    setAuthStatus("好友认证请求已发送", false);
}

QVariantMap ContactController::contactProfileByUid(int uid) const
{
    if (!_gateway || !_gateway->userMgr())
    {
        return {};
    }

    const auto friendInfo = _gateway->userMgr()->GetFriendById(uid);
    if (!friendInfo)
    {
        return {};
    }

    return {{"uid", friendInfo->_uid},
            {"userId", friendInfo->_user_id},
            {"name", friendInfo->_name},
            {"nick", friendInfo->_nick},
            {"icon", normalizeIconForQml(friendInfo->_icon)},
            {"desc", friendInfo->_desc},
            {"back", friendInfo->_back},
            {"sex", friendInfo->_sex}};
}

void ContactController::deleteFriend(int uid)
{
    if (!_gateway || !_gateway->userMgr() || !_gateway->chatTransport())
    {
        setAuthStatus("联系人状态异常，无法删除", true);
        return;
    }

    const auto selfInfo = _gateway->userMgr()->GetUserInfo();
    if (!selfInfo || selfInfo->_uid <= 0 || uid <= 0 || uid == selfInfo->_uid)
    {
        setAuthStatus("联系人状态异常，无法删除", true);
        return;
    }

    sendDeleteFriend(selfInfo->_uid, uid);
    setAuthStatus("正在删除联系人...", false);
}

void ContactController::showApplyRequests()
{
    ensureApplyInitialized();
    setContactPane(0);
    setAuthStatus(QString(), false);
}

void ContactController::jumpChatWithCurrentContact()
{
    if (_command_port.openCurrentContactChat)
    {
        _command_port.openCurrentContactChat();
    }
}

void ContactController::loadMoreContacts()
{
    ensureContactsInitialized();
    if (_contact_loading_more)
    {
        return;
    }
    if (!_gateway || !_gateway->userMgr())
    {
        setAuthStatus("用户状态异常，请重新登录", true);
        return;
    }

    if (_gateway->userMgr()->IsLoadConFin())
    {
        refreshContactLoadMoreState();
        return;
    }

    setContactLoadingMore(true);
    const auto contactList = _gateway->userMgr()->GetConListPerPage();
    appendContacts(contactList);
    _gateway->userMgr()->UpdateContactLoadedCount();
    setContactLoadingMore(false);
    refreshContactLoadMoreState();
}

void ContactController::clearAuthStatus()
{
    setAuthStatus(QString(), false);
}

bool ContactController::sendSearchUser(const QString& uidText, QString* errorText) const
{
    if (!_gateway || !_gateway->chatTransport())
    {
        if (errorText)
        {
            *errorText = "聊天连接未就绪";
        }
        return false;
    }

    if (!memochat::contact_payload::isValidUserId(uidText))
    {
        if (errorText)
        {
            *errorText = "请输入有效用户ID（格式 u#########）";
        }
        return false;
    }

    const QJsonObject jsonObj = memochat::contact_payload::buildSearchUserPayload(uidText);
    const QByteArray jsonData = QJsonDocument(jsonObj).toJson(QJsonDocument::Compact);
    _gateway->chatTransport()->slot_send_data(ReqId::ID_SEARCH_USER_REQ, jsonData);
    return true;
}

void ContactController::sendAddFriend(int selfUid,
                                      const QString& selfName,
                                      int targetUid,
                                      const QString& bakName,
                                      const QVariantList& labels) const
{
    const QJsonObject jsonObj =
        memochat::contact_payload::buildAddFriendPayload(selfUid, selfName, targetUid, bakName, labels);
    const QByteArray jsonData = QJsonDocument(jsonObj).toJson(QJsonDocument::Compact);
    _gateway->chatTransport()->slot_send_data(ReqId::ID_ADD_FRIEND_REQ, jsonData);
}

void ContactController::sendApproveFriend(int selfUid,
                                          int targetUid,
                                          const QString& remark,
                                          const QVariantList& labels) const
{
    const QJsonObject jsonObj =
        memochat::contact_payload::buildApproveFriendPayload(selfUid, targetUid, remark, labels);
    const QByteArray jsonData = QJsonDocument(jsonObj).toJson(QJsonDocument::Compact);
    _gateway->chatTransport()->slot_send_data(ReqId::ID_AUTH_FRIEND_REQ, jsonData);
}

void ContactController::sendDeleteFriend(int selfUid, int friendUid) const
{
    const QJsonObject jsonObj = memochat::contact_payload::buildDeleteFriendPayload(selfUid, friendUid);
    const QByteArray jsonData = QJsonDocument(jsonObj).toJson(QJsonDocument::Compact);
    _gateway->chatTransport()->slot_send_data(ReqId::ID_DELETE_FRIEND_REQ, jsonData);
}

void ContactController::setCommandPort(ContactCommandPort port)
{
    _command_port = std::move(port);
}

void ContactController::setBootstrapPort(ContactBootstrapPort port)
{
    _bootstrap_port = std::move(port);
}

void ContactController::setApplyBootstrapPort(ContactApplyBootstrapPort port)
{
    _apply_bootstrap_port = std::move(port);
}

void ContactController::syncModels(FriendListModel* contactListModel,
                                   SearchResultModel* searchResultModel,
                                   ApplyRequestModel* applyRequestModel)
{
    Q_UNUSED(contactListModel)
    Q_UNUSED(searchResultModel)
    Q_UNUSED(applyRequestModel)
}

void ContactController::syncContactPane(int pane)
{
    setContactPane(pane);
}

void ContactController::syncCurrentContact(int uid,
                                           const QString& name,
                                           const QString& nick,
                                           const QString& icon,
                                           const QString& back,
                                           int sex,
                                           const QString& userId)
{
    setCurrentContact(uid, name, nick, icon, back, sex, userId);
}

void ContactController::syncSearchPending(bool pending)
{
    setSearchPending(pending);
}

void ContactController::syncSearchStatus(const QString& text, bool isError)
{
    setSearchStatus(text, isError);
}

void ContactController::syncAuthStatus(const QString& text, bool isError)
{
    setAuthStatus(text, isError);
}

void ContactController::syncHasPendingApply(bool hasPending)
{
    setHasPendingApply(hasPending);
}

void ContactController::syncContactLoadingMore(bool loading)
{
    setContactLoadingMore(loading);
}

void ContactController::syncCanLoadMoreContacts(bool canLoad)
{
    setCanLoadMoreContacts(canLoad);
}

void ContactController::syncContactsReady(bool ready)
{
    setContactsReady(ready);
}

void ContactController::syncApplyReady(bool ready)
{
    setApplyReady(ready);
}

void ContactController::resetContactFeature()
{
    _contact_list_model->clear();
    _search_result_model->clear();
    _apply_request_model->clear();
    setContactPane(0);
    clearCurrentContact();
    setSearchPending(false);
    setSearchStatus(QString(), false);
    setAuthStatus(QString(), false);
    setHasPendingApply(false);
    setContactLoadingMore(false);
    setCanLoadMoreContacts(false);
    setContactsReady(false);
    setApplyReady(false);
}

void ContactController::setContactPane(int pane)
{
    if (_contact_pane == pane)
    {
        return;
    }

    _contact_pane = pane;
    emit contactPaneChanged();
}

void ContactController::clearCurrentContact()
{
    setCurrentContact(0, QString(), QString(), QStringLiteral("qrc:/res/head_1.jpg"), QString(), 0, QString());
}

void ContactController::setCurrentContact(int uid,
                                          const QString& name,
                                          const QString& nick,
                                          const QString& icon,
                                          const QString& back,
                                          int sex,
                                          const QString& userId)
{
    if (_current_contact_uid == uid && _current_contact_name == name && _current_contact_nick == nick &&
        _current_contact_icon == icon && _current_contact_back == back && _current_contact_sex == sex &&
        _current_contact_user_id == userId)
    {
        return;
    }

    _current_contact_uid = uid;
    _current_contact_name = name;
    _current_contact_nick = nick;
    _current_contact_icon = icon.isEmpty() ? QStringLiteral("qrc:/res/head_1.jpg") : icon;
    _current_contact_back = back;
    _current_contact_sex = sex;
    _current_contact_user_id = userId;
    emit currentContactChanged();
}

void ContactController::setContacts(const std::vector<std::shared_ptr<FriendInfo>>& contacts)
{
    _contact_list_model->setFriends(contacts);
}

void ContactController::appendContacts(const std::vector<std::shared_ptr<FriendInfo>>& contacts)
{
    _contact_list_model->appendFriends(contacts);
}

void ContactController::upsertContact(const std::shared_ptr<FriendInfo>& friendInfo)
{
    _contact_list_model->upsertFriend(friendInfo);
}

void ContactController::upsertContact(const std::shared_ptr<AuthInfo>& authInfo)
{
    _contact_list_model->upsertFriend(authInfo);
}

void ContactController::upsertContact(const std::shared_ptr<AuthRsp>& authRsp)
{
    _contact_list_model->upsertFriend(authRsp);
}

void ContactController::removeContactByUid(int uid)
{
    _contact_list_model->removeByUid(uid);
    if (_current_contact_uid == uid)
    {
        clearCurrentContact();
    }
}

void ContactController::setApplies(const std::vector<std::shared_ptr<ApplyInfo>>& applies)
{
    _apply_request_model->setApplies(applies);
}

void ContactController::upsertApply(const std::shared_ptr<ApplyInfo>& apply)
{
    _apply_request_model->upsertApply(apply);
}

void ContactController::upsertApply(const std::shared_ptr<AddFriendApply>& apply)
{
    _apply_request_model->upsertApply(apply);
}

void ContactController::markApplyApproved(int uid)
{
    _apply_request_model->markApproved(uid);
}

void ContactController::setApplyPending(int uid, bool pending)
{
    _apply_request_model->setPending(uid, pending);
}

void ContactController::clearSearchResultOnly()
{
    _search_result_model->clear();
}

void ContactController::setSearchResult(const std::shared_ptr<SearchInfo>& searchInfo)
{
    _search_result_model->setResult(searchInfo);
}

void ContactController::setSearchPending(bool pending)
{
    if (_search_pending == pending)
    {
        return;
    }

    _search_pending = pending;
    emit searchPendingChanged();
}

void ContactController::setSearchStatus(const QString& text, bool isError)
{
    if (_search_status_text == text && _search_status_error == isError)
    {
        return;
    }

    _search_status_text = text;
    _search_status_error = isError;
    emit searchStatusChanged();
}

void ContactController::setAuthStatus(const QString& text, bool isError)
{
    if (_auth_status_text == text && _auth_status_error == isError)
    {
        return;
    }

    _auth_status_text = text;
    _auth_status_error = isError;
    emit authStatusChanged();
}

void ContactController::setContactLoadingMore(bool loading)
{
    if (_contact_loading_more == loading)
    {
        return;
    }

    _contact_loading_more = loading;
    emit contactLoadingMoreChanged();
}

void ContactController::setCanLoadMoreContacts(bool canLoad)
{
    if (_can_load_more_contacts == canLoad)
    {
        return;
    }

    _can_load_more_contacts = canLoad;
    emit canLoadMoreContactsChanged();
}

void ContactController::setContactsReady(bool ready)
{
    if (_contacts_ready == ready)
    {
        return;
    }

    _contacts_ready = ready;
    emit contactsReadyChanged();
}

void ContactController::setApplyReady(bool ready)
{
    if (_apply_ready == ready)
    {
        return;
    }

    _apply_ready = ready;
    emit applyReadyChanged();
}

void ContactController::setHasPendingApply(bool hasPending)
{
    if (_has_pending_apply == hasPending)
    {
        return;
    }

    _has_pending_apply = hasPending;
    emit pendingApplyChanged();
}

void ContactController::refreshContactLoadMoreState()
{
    if (!_gateway || !_gateway->userMgr())
    {
        setCanLoadMoreContacts(false);
        return;
    }
    setCanLoadMoreContacts(!_gateway->userMgr()->IsLoadConFin());
}
