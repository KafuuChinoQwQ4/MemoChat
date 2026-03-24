#include "AppController.h"
#include "IconPathUtils.h"
#include "usermgr.h"
#include <QDateTime>
#include <QJsonDocument>
#include <QSettings>
#include <QVariantMap>
#include <QtGlobal>

namespace {
constexpr qint64 kPermChangeGroupInfo = 1LL << 0;
constexpr qint64 kPermInviteUsers = 1LL << 2;
constexpr qint64 kPermManageAdmins = 1LL << 3;
constexpr qint64 kPermBanUsers = 1LL << 5;
constexpr qint64 kDefaultAdminPermBits =
    (1LL << 0) | (1LL << 1) | (1LL << 2) | (1LL << 4) | (1LL << 5);
constexpr qint64 kOwnerPermBits = kDefaultAdminPermBits | (1LL << 3) | (1LL << 6);
}

QString AppController::tipText() const
{
    return _tip_text;
}

bool AppController::tipError() const
{
    return _tip_error;
}

bool AppController::busy() const
{
    return _busy;
}

bool AppController::registerSuccessPage() const
{
    return _register_success_page;
}

int AppController::registerCountdown() const
{
    return _register_countdown;
}

AppController::ChatTab AppController::chatTab() const
{
    return _chat_tab;
}

AppController::ContactPane AppController::contactPane() const
{
    return _contact_pane;
}

QString AppController::currentUserName() const
{
    return _current_user_name;
}

QString AppController::currentUserNick() const
{
    return _current_user_nick;
}

QString AppController::currentUserIcon() const
{
    return _current_user_icon;
}

QString AppController::currentUserDesc() const
{
    return _current_user_desc;
}

QString AppController::currentUserId() const
{
    return _current_user_id;
}

QString AppController::currentContactName() const
{
    return _current_contact_name;
}

QString AppController::currentContactNick() const
{
    return _current_contact_nick;
}

QString AppController::currentContactIcon() const
{
    return _current_contact_icon;
}

QString AppController::currentContactBack() const
{
    return _current_contact_back;
}

int AppController::currentContactSex() const
{
    return _current_contact_sex;
}

QString AppController::currentContactUserId() const
{
    return _current_contact_user_id;
}

bool AppController::hasCurrentContact() const
{
    return _current_contact_uid > 0;
}

QString AppController::currentChatPeerName() const
{
    return _current_chat_peer_name;
}

QString AppController::currentChatPeerIcon() const
{
    return _current_chat_peer_icon;
}

bool AppController::hasCurrentChat() const
{
    return _current_chat_uid > 0 || _current_group_id > 0;
}

bool AppController::hasCurrentGroup() const
{
    return _current_group_id > 0;
}

int AppController::currentGroupRole() const
{
    if (_current_group_id <= 0) {
        return 0;
    }
    auto groupInfo = _gateway.userMgr()->GetGroupById(_current_group_id);
    if (!groupInfo) {
        return 0;
    }
    return groupInfo->_role;
}

QString AppController::currentGroupName() const
{
    return _current_group_name;
}

QString AppController::currentGroupCode() const
{
    return _current_group_code;
}

bool AppController::currentGroupCanChangeInfo() const
{
    return hasCurrentGroupPermission(kPermChangeGroupInfo);
}

bool AppController::currentGroupCanInviteUsers() const
{
    return hasCurrentGroupPermission(kPermInviteUsers);
}

bool AppController::currentGroupCanManageAdmins() const
{
    return hasCurrentGroupPermission(kPermManageAdmins);
}

bool AppController::currentGroupCanBanUsers() const
{
    return hasCurrentGroupPermission(kPermBanUsers);
}

FriendListModel *AppController::dialogListModel()
{
    return &_dialog_list_model;
}

FriendListModel *AppController::chatListModel()
{
    return &_chat_list_model;
}

FriendListModel *AppController::groupListModel()
{
    return &_group_list_model;
}

FriendListModel *AppController::contactListModel()
{
    return &_contact_list_model;
}

ChatMessageModel *AppController::messageModel()
{
    return &_message_model;
}

SearchResultModel *AppController::searchResultModel()
{
    return &_search_result_model;
}

ApplyRequestModel *AppController::applyRequestModel()
{
    return &_apply_request_model;
}

MomentsModel *AppController::momentsModel() const
{
    return _moments_controller.model();
}

MomentsController *AppController::momentsController() const
{
    return const_cast<MomentsController*>(&_moments_controller);
}

bool AppController::searchPending() const
{
    return _search_pending;
}

QString AppController::searchStatusText() const
{
    return _search_status_text;
}

bool AppController::searchStatusError() const
{
    return _search_status_error;
}

bool AppController::hasPendingApply() const
{
    return _apply_request_model.hasUnapproved();
}

bool AppController::chatLoadingMore() const
{
    return _chat_loading_more;
}

bool AppController::canLoadMoreChats() const
{
    return _can_load_more_chats;
}

bool AppController::privateHistoryLoading() const
{
    return _private_history_loading;
}

bool AppController::canLoadMorePrivateHistory() const
{
    return _can_load_more_private_history;
}

bool AppController::contactLoadingMore() const
{
    return _contact_loading_more;
}

bool AppController::canLoadMoreContacts() const
{
    return _can_load_more_contacts;
}

QString AppController::authStatusText() const
{
    return _auth_status_text;
}

bool AppController::authStatusError() const
{
    return _auth_status_error;
}

QString AppController::settingsStatusText() const
{
    return _settings_status_text;
}

bool AppController::settingsStatusError() const
{
    return _settings_status_error;
}

QString AppController::groupStatusText() const
{
    return _group_status_text;
}

bool AppController::groupStatusError() const
{
    return _group_status_error;
}

bool AppController::mediaUploadInProgress() const
{
    return _media_upload_in_progress;
}

QString AppController::mediaUploadProgressText() const
{
    return _media_upload_progress_text;
}

QString AppController::currentDraftText() const
{
    return _current_draft_text;
}

QVariantList AppController::currentPendingAttachments() const
{
    return _current_pending_attachments;
}

bool AppController::hasPendingAttachments() const
{
    return !_current_pending_attachments.isEmpty();
}

bool AppController::currentDialogPinned() const
{
    return _current_dialog_pinned;
}

bool AppController::currentDialogMuted() const
{
    return _current_dialog_muted;
}

bool AppController::hasPendingReply() const
{
    return !_reply_to_msg_id.isEmpty();
}

QString AppController::replyTargetName() const
{
    return _reply_target_name;
}

QString AppController::replyPreviewText() const
{
    return _reply_preview_text;
}

bool AppController::dialogsReady() const
{
    return _dialogs_ready;
}

bool AppController::contactsReady() const
{
    return _contacts_ready;
}

bool AppController::groupsReady() const
{
    return _groups_ready;
}

bool AppController::applyReady() const
{
    return _apply_ready;
}

bool AppController::chatShellBusy() const
{
    return _page == ChatPage && !_dialogs_ready;
}

void AppController::setContactPane(ContactPane pane)
{
    if (_contact_pane == pane) {
        return;
    }

    _contact_pane = pane;
    emit contactPaneChanged();
}

void AppController::setCurrentContact(int uid, const QString &name, const QString &nick, const QString &icon,
                                      const QString &back, int sex, const QString &userId)
{
    const QString normalizedIcon = normalizeIconPath(icon);
    if (_current_contact_uid == uid
        && _current_contact_name == name
        && _current_contact_nick == nick
        && _current_contact_icon == normalizedIcon
        && _current_contact_back == back
        && _current_contact_sex == sex
        && _current_contact_user_id == userId) {
        return;
    }

    _current_contact_uid = uid;
    _current_contact_name = name;
    _current_contact_nick = nick;
    _current_contact_icon = normalizedIcon;
    _current_contact_back = back;
    _current_contact_sex = sex;
    _current_contact_user_id = userId;
    emit currentContactChanged();
}

void AppController::setCurrentChatPeerName(const QString &name)
{
    if (_current_chat_peer_name == name) {
        return;
    }

    _current_chat_peer_name = name;
    emit currentChatPeerChanged();
}

void AppController::setCurrentChatPeerIcon(const QString &icon)
{
    const QString normalizedIcon = normalizeIconPath(icon);
    if (_current_chat_peer_icon == normalizedIcon) {
        return;
    }

    _current_chat_peer_icon = normalizedIcon;
    emit currentChatPeerChanged();
}

void AppController::setSearchPending(bool pending)
{
    if (_search_pending == pending) {
        return;
    }

    _search_pending = pending;
    emit searchPendingChanged();
}

void AppController::setSearchStatus(const QString &text, bool isError)
{
    if (_search_status_text == text && _search_status_error == isError) {
        return;
    }

    _search_status_text = text;
    _search_status_error = isError;
    emit searchStatusChanged();
}

void AppController::clearSearchResultOnly()
{
    _search_result_model.clear();
}

void AppController::setChatLoadingMore(bool loading)
{
    if (_chat_loading_more == loading) {
        return;
    }

    _chat_loading_more = loading;
    emit chatLoadingMoreChanged();
}

void AppController::setPrivateHistoryLoading(bool loading)
{
    if (_private_history_loading == loading) {
        return;
    }
    _private_history_loading = loading;
    emit privateHistoryLoadingChanged();
}

void AppController::setCanLoadMorePrivateHistory(bool canLoad)
{
    if (_can_load_more_private_history == canLoad) {
        return;
    }
    _can_load_more_private_history = canLoad;
    emit canLoadMorePrivateHistoryChanged();
}

void AppController::setContactLoadingMore(bool loading)
{
    if (_contact_loading_more == loading) {
        return;
    }

    _contact_loading_more = loading;
    emit contactLoadingMoreChanged();
}

void AppController::setAuthStatus(const QString &text, bool isError)
{
    if (_auth_status_text == text && _auth_status_error == isError) {
        return;
    }

    _auth_status_text = text;
    _auth_status_error = isError;
    emit authStatusChanged();
}

void AppController::setSettingsStatus(const QString &text, bool isError)
{
    if (_settings_status_text == text && _settings_status_error == isError) {
        return;
    }

    _settings_status_text = text;
    _settings_status_error = isError;
    emit settingsStatusChanged();
}

void AppController::setCurrentGroup(qint64 groupId, const QString &name, const QString &groupCode)
{
    const QString normalizedName = (groupId > 0) ? name : QString();
    const QString normalizedCode = (groupId > 0) ? groupCode : QString();
    if (_current_group_id == groupId && _current_group_name == normalizedName && _current_group_code == normalizedCode) {
        return;
    }

    _current_group_id = groupId;
    _current_group_name = normalizedName;
    _current_group_code = normalizedCode;
    emit currentGroupChanged();
}

void AppController::setGroupStatus(const QString &text, bool isError)
{
    if (_group_status_text == text && _group_status_error == isError) {
        return;
    }

    _group_status_text = text;
    _group_status_error = isError;
    emit groupStatusChanged();
}

void AppController::setMediaUploadInProgress(bool inProgress)
{
    if (_media_upload_in_progress == inProgress) {
        return;
    }
    _media_upload_in_progress = inProgress;
    emit mediaUploadStateChanged();
}

void AppController::setMediaUploadProgressText(const QString &text)
{
    if (_media_upload_progress_text == text) {
        return;
    }
    _media_upload_progress_text = text;
    emit mediaUploadStateChanged();
}

void AppController::setCurrentDraftText(const QString &text)
{
    if (_current_draft_text == text) {
        return;
    }
    _current_draft_text = text;
    emit currentDraftTextChanged();
}

void AppController::setCurrentDialogPinned(bool pinned)
{
    if (_current_dialog_pinned == pinned) {
        return;
    }
    _current_dialog_pinned = pinned;
    emit currentDialogPinnedChanged();
}

void AppController::setCurrentDialogMuted(bool muted)
{
    if (_current_dialog_muted == muted) {
        return;
    }
    _current_dialog_muted = muted;
    emit currentDialogMutedChanged();
}

void AppController::setPendingReplyContext(const QString &msgId, const QString &senderName, const QString &previewText)
{
    const QString normalizedMsgId = msgId.trimmed();
    const QString normalizedSender = senderName.trimmed();
    QString normalizedPreview = previewText.trimmed();
    if (normalizedPreview.length() > 120) {
        normalizedPreview = normalizedPreview.left(120);
    }
    if (_reply_to_msg_id == normalizedMsgId
        && _reply_target_name == normalizedSender
        && _reply_preview_text == normalizedPreview) {
        return;
    }
    _reply_to_msg_id = normalizedMsgId;
    _reply_target_name = normalizedSender;
    _reply_preview_text = normalizedPreview;
    emit pendingReplyChanged();
}

int AppController::currentDialogUid() const
{
    if (_current_group_id > 0) {
        return -static_cast<int>(_current_group_id);
    }
    if (_current_chat_uid > 0) {
        return _current_chat_uid;
    }
    return 0;
}

void AppController::emitCurrentDialogUidChangedIfNeeded()
{
    const int dialogUid = currentDialogUid();
    if (_last_emitted_dialog_uid == dialogUid) {
        return;
    }
    _last_emitted_dialog_uid = dialogUid;
    emit currentDialogUidChanged();
}

bool AppController::resolveDialogTarget(int dialogUid, QString &dialogType, int &peerUid, qint64 &groupId) const
{
    dialogType.clear();
    peerUid = 0;
    groupId = 0;
    if (dialogUid == 0) {
        return false;
    }
    if (dialogUid > 0) {
        dialogType = "private";
        peerUid = dialogUid;
        return true;
    }

    const qint64 candidateGroupId = -static_cast<qint64>(dialogUid);
    if (candidateGroupId <= 0) {
        return false;
    }
    dialogType = "group";
    groupId = candidateGroupId;
    return true;
}

qint64 AppController::currentGroupPermissionBitsRaw() const
{
    if (_current_group_id <= 0) {
        return 0;
    }
    auto groupInfo = _gateway.userMgr()->GetGroupById(_current_group_id);
    if (!groupInfo) {
        return 0;
    }
    if (groupInfo->_role >= 3) {
        return kOwnerPermBits;
    }
    if (groupInfo->_role < 2) {
        return 0;
    }
    if (groupInfo->_permission_bits <= 0) {
        return kDefaultAdminPermBits;
    }
    return groupInfo->_permission_bits;
}

bool AppController::hasCurrentGroupPermission(qint64 permissionBit) const
{
    if (permissionBit <= 0) {
        return false;
    }
    return (currentGroupPermissionBitsRaw() & permissionBit) != 0;
}

qint64 AppController::latestGroupCreatedAt(qint64 groupId) const
{
    if (groupId <= 0) {
        return 0;
    }
    auto groupInfo = _gateway.userMgr()->GetGroupById(groupId);
    if (!groupInfo) {
        return 0;
    }
    qint64 latestTs = 0;
    for (const auto &one : groupInfo->_chat_msgs) {
        if (one) {
            latestTs = qMax(latestTs, one->_created_at);
        }
    }
    return latestTs;
}

qint64 AppController::latestPrivatePeerCreatedAt(int peerUid) const
{
    if (peerUid <= 0) {
        return 0;
    }
    auto friendInfo = _gateway.userMgr()->GetFriendById(peerUid);
    if (!friendInfo) {
        return 0;
    }
    qint64 latestTs = 0;
    for (const auto &one : friendInfo->_chat_msgs) {
        if (one && one->_from_uid == peerUid) {
            latestTs = qMax(latestTs, one->_created_at);
        }
    }
    return latestTs;
}

void AppController::syncCurrentDialogDraft()
{
    const int dialogUid = currentDialogUid();
    if (dialogUid == 0) {
        setCurrentDraftText("");
        setCurrentPendingAttachments(QVariantList());
        setCurrentDialogPinned(false);
        setCurrentDialogMuted(false);
        setPendingReplyContext(QString(), QString(), QString());
        return;
    }
    setCurrentDraftText(_dialog_draft_map.value(dialogUid));
    syncCurrentPendingAttachments();
    const bool pinned = _dialog_local_pinned_set.contains(dialogUid)
        || _dialog_server_pinned_map.value(dialogUid, 0) > 0;
    setCurrentDialogPinned(pinned);
    setCurrentDialogMuted(_dialog_server_mute_map.value(dialogUid, 0) > 0);
}

void AppController::syncCurrentPendingAttachments()
{
    const int dialogUid = currentDialogUid();
    if (dialogUid == 0) {
        setCurrentPendingAttachments(QVariantList());
        return;
    }
    setCurrentPendingAttachments(_dialog_pending_attachment_map.value(dialogUid));
}

void AppController::loadDraftStore(int ownerUid)
{
    _dialog_draft_map.clear();
    _dialog_local_pinned_set.clear();
    _dialog_server_pinned_map.clear();
    _dialog_server_mute_map.clear();
    _dialog_mention_map.clear();
    if (ownerUid <= 0) {
        return;
    }

    QSettings settings("MemoChat", "MemoChatQml");
    const QVariantMap serialized = settings.value(QString("chat_drafts/%1").arg(ownerUid)).toMap();
    for (auto it = serialized.cbegin(); it != serialized.cend(); ++it) {
        bool ok = false;
        const int dialogUid = it.key().toInt(&ok);
        if (!ok) {
            continue;
        }
        const QString draftText = it.value().toString();
        if (draftText.trimmed().isEmpty()) {
            continue;
        }
        _dialog_draft_map.insert(dialogUid, draftText);
    }

    const QStringList pinnedList = settings.value(QString("chat_pinned/%1").arg(ownerUid)).toStringList();
    for (const QString &entry : pinnedList) {
        bool ok = false;
        const int dialogUid = entry.toInt(&ok);
        if (ok && dialogUid != 0) {
            _dialog_local_pinned_set.insert(dialogUid);
        }
    }
}

void AppController::saveDraftStore(int ownerUid) const
{
    if (ownerUid <= 0) {
        return;
    }

    QVariantMap serialized;
    for (auto it = _dialog_draft_map.cbegin(); it != _dialog_draft_map.cend(); ++it) {
        if (it.value().trimmed().isEmpty()) {
            continue;
        }
        serialized.insert(QString::number(it.key()), it.value());
    }

    QSettings settings("MemoChat", "MemoChatQml");
    settings.setValue(QString("chat_drafts/%1").arg(ownerUid), serialized);
    QStringList pinnedList;
    pinnedList.reserve(_dialog_local_pinned_set.size());
    for (int uid : _dialog_local_pinned_set) {
        pinnedList.push_back(QString::number(uid));
    }
    settings.setValue(QString("chat_pinned/%1").arg(ownerUid), pinnedList);
}

void AppController::applyDraftToDialogModel(int dialogUid, const QString &draftText)
{
    const int idx = _dialog_list_model.indexOfUid(dialogUid);
    if (idx < 0) {
        return;
    }
    const QVariantMap item = _dialog_list_model.get(idx);
    _dialog_list_model.setDialogMeta(dialogUid,
                                     item.value("dialogType").toString(),
                                     item.value("unreadCount").toInt(),
                                     item.value("pinnedRank").toInt(),
                                     draftText,
                                     item.value("lastMsgTs").toLongLong(),
                                     item.value("muteState").toInt());
}

void AppController::refreshChatLoadMoreState()
{
    const bool canLoad = !_gateway.userMgr()->IsLoadChatFin();
    if (_can_load_more_chats == canLoad) {
        return;
    }

    _can_load_more_chats = canLoad;
    emit canLoadMoreChatsChanged();
}

void AppController::refreshContactLoadMoreState()
{
    const bool canLoad = !_gateway.userMgr()->IsLoadConFin();
    if (_can_load_more_contacts == canLoad) {
        return;
    }

    _can_load_more_contacts = canLoad;
    emit canLoadMoreContactsChanged();
}

void AppController::setTip(const QString &tip, bool isError)
{
    if (_tip_text == tip && _tip_error == isError) {
        return;
    }
    _tip_text = tip;
    _tip_error = isError;
    emit tipChanged();
}

void AppController::setBusy(bool value)
{
    if (_busy == value) {
        return;
    }
    _busy = value;
    emit busyChanged();
}

void AppController::setPage(Page newPage)
{
    if (_page == newPage) {
        return;
    }
    _page = newPage;
    emit pageChanged();
}

QString AppController::normalizeIconPath(QString icon) const
{
    return normalizeIconForQml(icon);
}

void AppController::setDialogsReady(bool ready)
{
    if (_dialogs_ready == ready) {
        return;
    }
    _dialogs_ready = ready;
    emit lazyBootstrapStateChanged();
}

void AppController::setContactsReady(bool ready)
{
    if (_contacts_ready == ready) {
        return;
    }
    _contacts_ready = ready;
    emit lazyBootstrapStateChanged();
}

void AppController::setGroupsReady(bool ready)
{
    if (_groups_ready == ready) {
        return;
    }
    _groups_ready = ready;
    emit lazyBootstrapStateChanged();
}

void AppController::setApplyReady(bool ready)
{
    if (_apply_ready == ready) {
        return;
    }
    _apply_ready = ready;
    emit lazyBootstrapStateChanged();
}
