#include "AppController.h"
#include "IconPathUtils.h"

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
