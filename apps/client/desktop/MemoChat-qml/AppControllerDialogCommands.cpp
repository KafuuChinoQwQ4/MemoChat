#include "AppController.h"
#include "ConversationSyncService.h"
#include "IChatTransport.h"
#include "usermgr.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVariantMap>

void AppController::updateCurrentDraft(const QString &text)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    const int dialogUid = currentDialogUid();
    if (!selfInfo || dialogUid == 0) {
        return;
    }

    QString nextDraft = text;
    if (nextDraft.size() > 2000) {
        nextDraft = nextDraft.left(2000);
    }
    const QString normalizedDraft = nextDraft.trimmed().isEmpty() ? QString() : nextDraft;
    if (normalizedDraft.isEmpty()) {
        _dialog_draft_map.remove(dialogUid);
    } else {
        _dialog_draft_map.insert(dialogUid, normalizedDraft);
    }

    setCurrentDraftText(normalizedDraft);
    applyDraftToDialogModel(dialogUid, normalizedDraft);
    const int ownerUid = _gateway.userMgr()->GetUid();
    if (ownerUid > 0) {
        saveDraftStore(ownerUid);
    }

    QJsonObject obj;
    obj["fromuid"] = selfInfo->_uid;
    if (_current_group_id > 0) {
        obj["dialog_type"] = "group";
        obj["groupid"] = static_cast<qint64>(_current_group_id);
    } else if (_current_chat_uid > 0) {
        obj["dialog_type"] = "private";
        obj["peer_uid"] = _current_chat_uid;
    } else {
        return;
    }
    obj["draft_text"] = normalizedDraft;
    const QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    _gateway.chatTransport()->slot_send_data(ReqId::ID_SYNC_DRAFT_REQ, payload);
}

void AppController::toggleCurrentDialogPinned()
{
    const int dialogUid = currentDialogUid();
    toggleDialogPinnedByUid(dialogUid);
}

void AppController::toggleCurrentDialogMuted()
{
    const int dialogUid = currentDialogUid();
    toggleDialogMutedByUid(dialogUid);
}

void AppController::toggleDialogPinnedByUid(int dialogUid)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo || dialogUid == 0) {
        return;
    }

    QString dialogType;
    int peerUid = 0;
    qint64 groupId = 0;
    if (!resolveDialogTarget(dialogUid, dialogType, peerUid, groupId)) {
        return;
    }

    const bool currentlyPinned = _dialog_local_pinned_set.contains(dialogUid)
        || _dialog_server_pinned_map.value(dialogUid, 0) > 0;
    const bool nextPinned = !currentlyPinned;
    if (!nextPinned) {
        _dialog_local_pinned_set.remove(dialogUid);
        _dialog_server_pinned_map.insert(dialogUid, 0);
    } else {
        _dialog_local_pinned_set.insert(dialogUid);
        _dialog_server_pinned_map.insert(dialogUid,
                                         static_cast<int>(QDateTime::currentSecsSinceEpoch()));
    }
    if (dialogUid == currentDialogUid()) {
        setCurrentDialogPinned(nextPinned);
    }

    const int ownerUid = _gateway.userMgr()->GetUid();
    if (ownerUid > 0) {
        saveDraftStore(ownerUid);
    }

    QJsonObject obj;
    obj["fromuid"] = selfInfo->_uid;
    obj["dialog_type"] = dialogType;
    if (dialogType == "group") {
        obj["groupid"] = static_cast<qint64>(groupId);
    } else {
        obj["peer_uid"] = peerUid;
    }
    obj["pinned_rank"] = nextPinned ? static_cast<int>(QDateTime::currentSecsSinceEpoch()) : 0;
    const QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    _gateway.chatTransport()->slot_send_data(ReqId::ID_PIN_DIALOG_REQ, payload);

    refreshDialogModel();
}

void AppController::toggleDialogMutedByUid(int dialogUid)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo || dialogUid == 0) {
        return;
    }

    QString dialogType;
    int peerUid = 0;
    qint64 groupId = 0;
    if (!resolveDialogTarget(dialogUid, dialogType, peerUid, groupId)) {
        return;
    }

    const bool currentlyMuted = _dialog_server_mute_map.value(dialogUid, 0) > 0;
    const int nextMuteState = currentlyMuted ? 0 : 1;
    _dialog_server_mute_map.insert(dialogUid, nextMuteState);
    if (dialogUid == currentDialogUid()) {
        setCurrentDialogMuted(nextMuteState > 0);
    }

    const int idx = _dialog_list_model.indexOfUid(dialogUid);
    if (idx >= 0) {
        const QVariantMap item = _dialog_list_model.get(idx);
        _dialog_list_model.setDialogMeta(dialogUid,
                                         item.value("dialogType").toString(),
                                         item.value("unreadCount").toInt(),
                                         item.value("pinnedRank").toInt(),
                                         item.value("draftText").toString(),
                                         item.value("lastMsgTs").toLongLong(),
                                         nextMuteState);
    }

    QJsonObject obj;
    obj["fromuid"] = selfInfo->_uid;
    obj["dialog_type"] = dialogType;
    if (dialogType == "group") {
        obj["groupid"] = static_cast<qint64>(groupId);
    } else {
        obj["peer_uid"] = peerUid;
    }
    obj["draft_text"] = _dialog_draft_map.value(dialogUid);
    obj["mute_state"] = nextMuteState;
    const QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    _gateway.chatTransport()->slot_send_data(ReqId::ID_SYNC_DRAFT_REQ, payload);
}

void AppController::markDialogReadByUid(int dialogUid)
{
    if (dialogUid == 0) {
        return;
    }

    _dialog_list_model.clearUnread(dialogUid);
    _dialog_list_model.clearMention(dialogUid);
    _dialog_mention_map.remove(dialogUid);
    if (dialogUid > 0) {
        _chat_list_model.clearUnread(dialogUid);
        const qint64 latestPeerTs = latestPrivatePeerCreatedAt(dialogUid);
        if (latestPeerTs > 0) {
            sendPrivateReadAck(dialogUid, latestPeerTs);
        } else {
            sendPrivateReadAck(dialogUid, 0);
        }
        return;
    }

    _group_list_model.clearUnread(dialogUid);
    const qint64 groupId = ConversationSyncService::groupIdForDialogUid(_group_uid_map, dialogUid);
    if (groupId <= 0) {
        return;
    }
    const qint64 latestGroupTs = latestGroupCreatedAt(groupId);
    if (latestGroupTs > 0) {
        sendGroupReadAck(groupId, latestGroupTs);
    } else {
        sendGroupReadAck(groupId, 0);
    }
}

void AppController::clearDialogDraftByUid(int dialogUid)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo || dialogUid == 0) {
        return;
    }

    QString dialogType;
    int peerUid = 0;
    qint64 groupId = 0;
    if (!resolveDialogTarget(dialogUid, dialogType, peerUid, groupId)) {
        return;
    }

    _dialog_draft_map.remove(dialogUid);
    applyDraftToDialogModel(dialogUid, QString());
    if (dialogUid == currentDialogUid()) {
        setCurrentDraftText(QString());
    }
    const int ownerUid = _gateway.userMgr()->GetUid();
    if (ownerUid > 0) {
        saveDraftStore(ownerUid);
    }

    QJsonObject obj;
    obj["fromuid"] = selfInfo->_uid;
    obj["dialog_type"] = dialogType;
    if (dialogType == "group") {
        obj["groupid"] = static_cast<qint64>(groupId);
    } else {
        obj["peer_uid"] = peerUid;
    }
    obj["draft_text"] = QString();
    const QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    _gateway.chatTransport()->slot_send_data(ReqId::ID_SYNC_DRAFT_REQ, payload);
}

void AppController::beginReplyMessage(const QString &msgId, const QString &senderName, const QString &previewText)
{
    const QString normalizedMsgId = msgId.trimmed();
    if (normalizedMsgId.isEmpty()) {
        return;
    }
    QString normalizedPreview = previewText.trimmed();
    if (normalizedPreview.length() > 120) {
        normalizedPreview = normalizedPreview.left(120);
    }
    setPendingReplyContext(normalizedMsgId, senderName.trimmed(), normalizedPreview);
}

void AppController::cancelReplyMessage()
{
    setPendingReplyContext(QString(), QString(), QString());
}
