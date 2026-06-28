#include "ContactController.h"
#include "ApplyRequestModel.h"
#include "ClientGateway.h"
#include "ContactRequestPayloads.h"
#include "FriendListModel.h"
#include "IChatTransport.h"
#include "IconPathUtils.h"
#include "SearchResultModel.h"
#include "global.h"
#include "httpmgr.h"
#include "usermgr.h"
#include <QJsonDocument>
#include <QJsonParseError>
#include <QUrl>

#include <utility>

namespace
{
void assignNonEmpty(QString& target, const QString& next)
{
    if (!next.trimmed().isEmpty())
    {
        target = next;
    }
}

QVariantMap friendProfileMap(const std::shared_ptr<FriendInfo>& friendInfo)
{
    if (!friendInfo)
    {
        return {};
    }

    return {{"uid", friendInfo->_uid},
            {"isFriend", true},
            {"userId", friendInfo->_user_id},
            {"name", friendInfo->_name},
            {"nick", friendInfo->_nick},
            {"icon", normalizeIconForQml(friendInfo->_icon)},
            {"desc", friendInfo->_desc},
            {"back", friendInfo->_back},
            {"sex", friendInfo->_sex}};
}

QVariantMap publicProfileMapFromJson(const QJsonObject& obj)
{
    const int uid = obj.value(QStringLiteral("uid")).toInt();
    if (uid <= 0)
    {
        return {};
    }
    const QString icon = obj.value(QStringLiteral("icon")).toString().trimmed();

    return {{"uid", uid},
            {"isFriend", false},
            {"userId", obj.value(QStringLiteral("user_id")).toString().trimmed()},
             {"name", obj.value(QStringLiteral("name")).toString()},
              {"nick", obj.value(QStringLiteral("nick")).toString()},
               {"icon", icon.isEmpty() ? QString() : normalizeIconForQml(icon)},
               {"desc", obj.value(QStringLiteral("desc")).toString()},
                {"back", QString()},
                {"sex", obj.value(QStringLiteral("sex")).toInt()}};
}

void mergeCachedPublicFields(QVariantMap& profile, const QVariantMap& cached)
{
    if (cached.isEmpty())
    {
        return;
    }

    for (const QString& key : {QStringLiteral("userId"),
             QStringLiteral("name"), QStringLiteral("nick"), QStringLiteral("icon"), QStringLiteral("desc")})
    {
        if (profile.value(key).toString().trimmed().isEmpty() && !cached.value(key).toString().trimmed().isEmpty())
        {
            profile.insert(key, cached.value(key));
        }
    }
    if (profile.value(QStringLiteral("sex")).toInt() == 0 && cached.value(QStringLiteral("sex")).toInt() != 0)
    {
        profile.insert(QStringLiteral("sex"), cached.value(QStringLiteral("sex")));
    }
}

std::vector<int> visibleContactUidsMissingPublicId(FriendListModel* model)
{
    std::vector<int> uids;
    if (!model)
    {
        return uids;
    }

    for (int i = 0; i < model->rowCount(); ++i)
    {
        const QVariantMap row = model->get(i);
        const int uid = row.value(QStringLiteral("uid")).toInt();
        if (uid > 0 && row.value(QStringLiteral("userId")).toString().trimmed().isEmpty())
        {
            uids.push_back(uid);
        }
    }
    return uids;
}
} // namespace

ContactController::ContactController(ClientGateway* gateway, QObject* parent)
    : QObject(parent)
    , _gateway(gateway)
    , _contact_list_model(new FriendListModel(this))
    , _search_result_model(new SearchResultModel(this))
    , _apply_request_model(new ApplyRequestModel(this))
{
    _contact_list_model->setContactSectionOrderingEnabled(true);
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
    const bool modelHasContacts = _contact_list_model && _contact_list_model->rowCount() > 0;
    const bool modelMissingPublicIds = !visibleContactUidsMissingPublicId(_contact_list_model).empty();
    if (_contacts_ready && modelHasContacts && !modelMissingPublicIds)
    {
        return;
    }

    const std::vector<std::shared_ptr<FriendInfo>> snapshotContacts =
        _bootstrap_port.friendSnapshot ? _bootstrap_port.friendSnapshot() : std::vector<std::shared_ptr<FriendInfo>>{};
    if (_contacts_ready)
    {
        if (!snapshotContacts.empty())
        {
            if (modelHasContacts)
            {
                bool updatedVisibleContacts = false;
                for (const auto& friendInfo : snapshotContacts)
                {
                    if (friendInfo && _contact_list_model->indexOfUid(friendInfo->_uid) >= 0)
                    {
                        _contact_list_model->upsertFriend(friendInfo);
                        updatedVisibleContacts = true;
                    }
                }
                if (updatedVisibleContacts)
                {
                    refreshCurrentContactFromStore();
                    emit contactProfilesChanged();
                }
            }
            else
            {
                setContacts(snapshotContacts);
            }
            refreshContactLoadMoreState();
        }
        for (const int uid : visibleContactUidsMissingPublicId(_contact_list_model))
        {
            requestPublicProfileByUid(uid);
        }
        return;
    }

    bool loadedAnyContacts = !snapshotContacts.empty();
    if (_bootstrap_port.ensureChatListInitialized)
    {
        _bootstrap_port.ensureChatListInitialized();
    }
    if (!snapshotContacts.empty())
    {
        setContacts(snapshotContacts);
        refreshContactLoadMoreState();
    }
    else
    {
        const std::vector<std::shared_ptr<FriendInfo>> contacts =
            _bootstrap_port.nextPage ? _bootstrap_port.nextPage() : std::vector<std::shared_ptr<FriendInfo>>{};
        setContacts(contacts);
        loadedAnyContacts = !contacts.empty();
        if (_bootstrap_port.markPageLoaded)
        {
            _bootstrap_port.markPageLoaded();
        }
        if (_bootstrap_port.loadFinished)
        {
            setCanLoadMoreContacts(!_bootstrap_port.loadFinished());
        }
    }
    if (!loadedAnyContacts && _command_port.requestRelationBootstrap)
    {
        _command_port.requestRelationBootstrap();
    }
    setContactsReady(loadedAnyContacts);
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
    const auto cachedIt = _profile_lookup_cache.constFind(uid);
    const QVariantMap cached = cachedIt == _profile_lookup_cache.constEnd() ? QVariantMap() : cachedIt.value();

    if (!_gateway || !_gateway->userMgr())
    {
        return cached;
    }

    const auto friendInfo = _gateway->userMgr()->GetFriendById(uid);
    if (!friendInfo)
    {
        return cached;
    }

    QVariantMap profile = friendProfileMap(friendInfo);
    mergeCachedPublicFields(profile, cached);
    profile.insert(QStringLiteral("isFriend"), true);
    return profile;
}

void ContactController::refreshContactProfileByUid(int uid)
{
    if (uid <= 0)
    {
        return;
    }

    bool alreadyHasPublicId = false;
    if (_gateway && _gateway->userMgr())
    {
        const auto friendInfo = _gateway->userMgr()->GetFriendById(uid);
        if (friendInfo)
        {
            alreadyHasPublicId = !friendInfo->_user_id.trimmed().isEmpty();
            if (_contact_list_model->indexOfUid(uid) >= 0)
            {
                _contact_list_model->upsertFriend(friendInfo);
            }
            refreshCurrentContactFromStore();
            emit contactProfilesChanged();
        }
    }

    if (alreadyHasPublicId)
    {
        return;
    }

    ensureContactsInitialized();
    if (_command_port.requestRelationBootstrap)
    {
        _command_port.requestRelationBootstrap();
    }
    requestPublicProfileByUid(uid);
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

    refreshContactLoadMoreState();
    if (!_can_load_more_contacts)
    {
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

void ContactController::handleContactHttpFinished(ReqId id, const QString& res, ErrorCodes err)
{
    if (id != ReqId::ID_GET_USER_INFO)
    {
        return;
    }

    if (err != ErrorCodes::SUCCESS)
    {
        _profile_lookup_pending_uids.clear();
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(res.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject())
    {
        _profile_lookup_pending_uids.clear();
        return;
    }

    const QJsonObject obj = doc.object();
    const int error = obj.value(QStringLiteral("error")).toInt(ErrorCodes::ERR_JSON);
    const int uid = obj.value(QStringLiteral("uid")).toInt();
    if (uid > 0)
    {
        _profile_lookup_pending_uids.remove(uid);
    }
    if (error != ErrorCodes::SUCCESS || uid <= 0)
    {
        if (uid <= 0)
        {
            _profile_lookup_pending_uids.clear();
        }
        return;
    }

    const QVariantMap profile = publicProfileMapFromJson(obj);
    if (profile.isEmpty())
    {
        return;
    }
    _profile_lookup_cache.insert(uid, profile);

    if (_gateway && _gateway->userMgr())
    {
        const auto friendInfo = _gateway->userMgr()->GetFriendById(uid);
        if (friendInfo)
        {
            assignNonEmpty(friendInfo->_user_id, profile.value(QStringLiteral("userId")).toString());
            assignNonEmpty(friendInfo->_name, profile.value(QStringLiteral("name")).toString());
            assignNonEmpty(friendInfo->_nick, profile.value(QStringLiteral("nick")).toString());
            assignNonEmpty(friendInfo->_icon, profile.value(QStringLiteral("icon")).toString());
            assignNonEmpty(friendInfo->_desc, profile.value(QStringLiteral("desc")).toString());
            if (profile.value(QStringLiteral("sex")).toInt() != 0)
            {
                friendInfo->_sex = profile.value(QStringLiteral("sex")).toInt();
            }
            if (_contact_list_model->indexOfUid(uid) >= 0)
            {
                _contact_list_model->upsertFriend(friendInfo);
            }
        }
    }

    refreshCurrentContactFromStore();
    emit contactProfilesChanged();
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
    refreshCurrentContactFromStore();
    emit contactProfilesChanged();
}

void ContactController::appendContacts(const std::vector<std::shared_ptr<FriendInfo>>& contacts)
{
    _contact_list_model->appendFriends(contacts);
    refreshCurrentContactFromStore();
    emit contactProfilesChanged();
}

void ContactController::upsertContact(const std::shared_ptr<FriendInfo>& friendInfo)
{
    _contact_list_model->upsertFriend(friendInfo);
    refreshCurrentContactFromStore();
    emit contactProfilesChanged();
}

void ContactController::upsertContact(const std::shared_ptr<AuthInfo>& authInfo)
{
    _contact_list_model->upsertFriend(authInfo);
    refreshCurrentContactFromStore();
    emit contactProfilesChanged();
}

void ContactController::upsertContact(const std::shared_ptr<AuthRsp>& authRsp)
{
    _contact_list_model->upsertFriend(authRsp);
    refreshCurrentContactFromStore();
    emit contactProfilesChanged();
}

void ContactController::refreshContactProfiles()
{
    if (_gateway && _gateway->userMgr())
    {
        const auto friends = _gateway->userMgr()->GetFriendListSnapshot();
        for (const auto& friendInfo : friends)
        {
            if (friendInfo && _contact_list_model->indexOfUid(friendInfo->_uid) >= 0)
            {
                _contact_list_model->upsertFriend(friendInfo);
            }
        }
    }
    refreshCurrentContactFromStore();
    emit contactProfilesChanged();
}

void ContactController::removeContactByUid(int uid)
{
    _contact_list_model->removeByUid(uid);
    if (_current_contact_uid == uid)
    {
        clearCurrentContact();
    }
    emit contactProfilesChanged();
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
    const int visibleContacts = _contact_list_model ? _contact_list_model->rowCount() : 0;
    const int totalContacts = static_cast<int>(_gateway->userMgr()->GetFriendListSnapshot().size());
    setCanLoadMoreContacts(visibleContacts < totalContacts && !_gateway->userMgr()->IsLoadConFin());
}

void ContactController::refreshCurrentContactFromStore()
{
    if (_current_contact_uid <= 0 || !_gateway || !_gateway->userMgr())
    {
        return;
    }

    const auto friendInfo = _gateway->userMgr()->GetFriendById(_current_contact_uid);
    if (!friendInfo)
    {
        return;
    }

    setCurrentContact(friendInfo->_uid,
                      friendInfo->_name,
                      friendInfo->_nick,
                      normalizeIconForQml(friendInfo->_icon),
                      friendInfo->_back,
                      friendInfo->_sex,
                      friendInfo->_user_id);
}

void ContactController::requestPublicProfileByUid(int uid)
{
    if (uid <= 0 || _profile_lookup_pending_uids.contains(uid) || !_gateway || !_gateway->httpMgr())
    {
        return;
    }

    QJsonObject payload;
    payload.insert(QStringLiteral("uid"), uid);
    _profile_lookup_pending_uids.insert(uid);
    _gateway->httpMgr()->PostHttpReq(QUrl(gate_url_prefix + QStringLiteral("/get_user_info")),
                                          payload,
                                          ReqId::ID_GET_USER_INFO,
                                          Modules::CONTACTMOD,
                                          QStringLiteral("contact-profile"));
}
