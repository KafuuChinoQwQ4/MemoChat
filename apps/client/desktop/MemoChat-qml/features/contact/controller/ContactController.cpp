#include "ContactController.h"
#include "ClientGateway.h"
#include "IChatTransport.h"
#include "IconPathUtils.h"
#include "global.h"
#include "usermgr.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

namespace
{
QJsonArray buildLabelsArray(const QVariantList& labels)
{
    QJsonArray labelsArray;
    for (const auto& labelVar : labels)
    {
        const QString label = labelVar.toString().trimmed();
        if (!label.isEmpty())
        {
            labelsArray.append(label);
        }
    }
    return labelsArray;
}
} // namespace

ContactController::ContactController(ClientGateway* gateway, QObject* parent)
    : QObject(parent)
    , _gateway(gateway)
{
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

void ContactController::ensureContactsInitialized()
{
    emit ensureContactsInitializedRequested();
}

void ContactController::ensureApplyInitialized()
{
    emit ensureApplyInitializedRequested();
}

void ContactController::selectContactIndex(int index)
{
    emit selectContactIndexRequested(index);
}

void ContactController::searchUser(const QString& uidText)
{
    emit searchUserRequested(uidText);
}

void ContactController::clearSearchState()
{
    emit clearSearchStateRequested();
}

void ContactController::requestAddFriend(int uid, const QString& bakName, const QVariantList& labels)
{
    emit requestAddFriendRequested(uid, bakName, labels);
}

void ContactController::approveFriend(int uid, const QString& backName, const QVariantList& labels)
{
    emit approveFriendRequested(uid, backName, labels);
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
    emit deleteFriendRequested(uid);
}

void ContactController::showApplyRequests()
{
    emit showApplyRequestsRequested();
}

void ContactController::jumpChatWithCurrentContact()
{
    emit jumpChatWithCurrentContactRequested();
}

void ContactController::loadMoreContacts()
{
    emit loadMoreContactsRequested();
}

void ContactController::clearAuthStatus()
{
    emit clearAuthStatusRequested();
}

bool ContactController::sendSearchUser(const QString& uidText, QString* errorText) const
{
    static const QRegularExpression kUserIdPattern("^u[1-9][0-9]{8}$");
    const QString userId = uidText.trimmed();
    if (!kUserIdPattern.match(userId).hasMatch())
    {
        if (errorText)
        {
            *errorText = "请输入有效用户ID（格式 u#########）";
        }
        return false;
    }

    QJsonObject jsonObj;
    jsonObj["user_id"] = userId;
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
    QJsonObject jsonObj;
    jsonObj["uid"] = selfUid;
    jsonObj["applyname"] = selfName;
    jsonObj["bakname"] = bakName.trimmed().isEmpty() ? selfName : bakName.trimmed();
    jsonObj["touid"] = targetUid;
    jsonObj["labels"] = buildLabelsArray(labels);

    const QByteArray jsonData = QJsonDocument(jsonObj).toJson(QJsonDocument::Compact);
    _gateway->chatTransport()->slot_send_data(ReqId::ID_ADD_FRIEND_REQ, jsonData);
}

void ContactController::sendApproveFriend(int selfUid,
                                          int targetUid,
                                          const QString& remark,
                                          const QVariantList& labels) const
{
    QJsonObject jsonObj;
    jsonObj["fromuid"] = selfUid;
    jsonObj["touid"] = targetUid;
    jsonObj["back"] = remark;
    jsonObj["labels"] = buildLabelsArray(labels);

    const QByteArray jsonData = QJsonDocument(jsonObj).toJson(QJsonDocument::Compact);
    _gateway->chatTransport()->slot_send_data(ReqId::ID_AUTH_FRIEND_REQ, jsonData);
}

void ContactController::syncModels(FriendListModel* contactListModel,
                                   SearchResultModel* searchResultModel,
                                   ApplyRequestModel* applyRequestModel)
{
    if (_contact_list_model == contactListModel && _search_result_model == searchResultModel &&
        _apply_request_model == applyRequestModel)
    {
        return;
    }

    _contact_list_model = contactListModel;
    _search_result_model = searchResultModel;
    _apply_request_model = applyRequestModel;
    emit modelChanged();
}

void ContactController::syncContactPane(int pane)
{
    if (_contact_pane == pane)
    {
        return;
    }

    _contact_pane = pane;
    emit contactPaneChanged();
}

void ContactController::syncCurrentContact(int uid,
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
    _current_contact_icon = icon;
    _current_contact_back = back;
    _current_contact_sex = sex;
    _current_contact_user_id = userId;
    emit currentContactChanged();
}

void ContactController::syncSearchPending(bool pending)
{
    if (_search_pending == pending)
    {
        return;
    }

    _search_pending = pending;
    emit searchPendingChanged();
}

void ContactController::syncSearchStatus(const QString& text, bool isError)
{
    if (_search_status_text == text && _search_status_error == isError)
    {
        return;
    }

    _search_status_text = text;
    _search_status_error = isError;
    emit searchStatusChanged();
}

void ContactController::syncAuthStatus(const QString& text, bool isError)
{
    if (_auth_status_text == text && _auth_status_error == isError)
    {
        return;
    }

    _auth_status_text = text;
    _auth_status_error = isError;
    emit authStatusChanged();
}

void ContactController::syncHasPendingApply(bool hasPending)
{
    if (_has_pending_apply == hasPending)
    {
        return;
    }

    _has_pending_apply = hasPending;
    emit pendingApplyChanged();
}

void ContactController::syncContactLoadingMore(bool loading)
{
    if (_contact_loading_more == loading)
    {
        return;
    }

    _contact_loading_more = loading;
    emit contactLoadingMoreChanged();
}

void ContactController::syncCanLoadMoreContacts(bool canLoad)
{
    if (_can_load_more_contacts == canLoad)
    {
        return;
    }

    _can_load_more_contacts = canLoad;
    emit canLoadMoreContactsChanged();
}

void ContactController::syncContactsReady(bool ready)
{
    if (_contacts_ready == ready)
    {
        return;
    }

    _contacts_ready = ready;
    emit contactsReadyChanged();
}
