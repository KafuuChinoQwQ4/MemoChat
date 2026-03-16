#include "ChatMessageDispatcher.h"

#include <QDateTime>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

#include "usermgr.h"

ChatMessageDispatcher::ChatMessageDispatcher(QObject *parent)
    : QObject(parent)
{
    initHandlers();
}

void ChatMessageDispatcher::initHandlers()
{
    _handlers.insert(ID_CHAT_LOGIN_RSP, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug() << "handle id is " << id;

        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if (jsonDoc.isNull()) {
            qWarning() << "chat login rsp json parse failed";
            emit sig_login_failed(ErrorCodes::ERR_JSON);
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();
        qDebug() << "data jsonobj is " << jsonObj;

        if (!jsonObj.contains("error")) {
            emit sig_login_failed(ErrorCodes::ERR_JSON);
            return;
        }

        const int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS) {
            qWarning() << "chat login rsp failed, err:" << err;
            emit sig_login_failed(err);
            return;
        }

        auto user_info = std::make_shared<UserInfo>(jsonObj["uid"].toInt(),
                                                    jsonObj["name"].toString(),
                                                    jsonObj["nick"].toString(),
                                                    jsonObj["icon"].toString(),
                                                    jsonObj["sex"].toInt(),
                                                    "",
                                                    jsonObj["desc"].toString(),
                                                    jsonObj["user_id"].toString());
        UserMgr::GetInstance()->SetUserInfo(user_info);
        const QString responseToken = jsonObj["token"].toString();
        if (!responseToken.isEmpty()) {
            UserMgr::GetInstance()->SetToken(responseToken);
        }
        if (jsonObj.contains("apply_list")) {
            UserMgr::GetInstance()->AppendApplyList(jsonObj["apply_list"].toArray());
        }
        if (jsonObj.contains("friend_list")) {
            UserMgr::GetInstance()->AppendFriendList(jsonObj["friend_list"].toArray());
        }

        emit sig_swich_chatdlg();
    });

    _handlers.insert(ID_GET_RELATION_BOOTSTRAP_RSP, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(id);
        Q_UNUSED(len);
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if (jsonDoc.isNull() || !jsonDoc.isObject()) {
            qWarning() << "relation bootstrap rsp json parse failed";
            return;
        }

        const QJsonObject jsonObj = jsonDoc.object();
        const int err = jsonObj.value("error").toInt(ErrorCodes::ERR_JSON);
        if (err != ErrorCodes::SUCCESS) {
            qWarning() << "relation bootstrap rsp failed, err:" << err;
            return;
        }

        if (jsonObj.contains("apply_list")) {
            UserMgr::GetInstance()->AppendApplyList(jsonObj.value("apply_list").toArray());
        }
        if (jsonObj.contains("friend_list")) {
            UserMgr::GetInstance()->AppendFriendList(jsonObj.value("friend_list").toArray());
        }

        emit sig_relation_bootstrap_updated();
    });

    _handlers.insert(ID_SEARCH_USER_RSP, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug() << "handle id is " << id << " data is " << data;

        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if (jsonDoc.isNull()) {
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();
        if (jsonObj.value("error").toInt(ErrorCodes::ERR_JSON) != ErrorCodes::SUCCESS) {
            emit sig_user_search(nullptr);
            return;
        }

        auto search_info = std::make_shared<SearchInfo>(jsonObj["uid"].toInt(),
                                                        jsonObj["name"].toString(),
                                                        jsonObj["nick"].toString(),
                                                        jsonObj["desc"].toString(),
                                                        jsonObj["sex"].toInt(),
                                                        jsonObj["icon"].toString(),
                                                        jsonObj["user_id"].toString());
        emit sig_user_search(search_info);
    });

    _handlers.insert(ID_NOTIFY_ADD_FRIEND_REQ, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug() << "handle id is " << id << " data is " << data;

        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if (jsonDoc.isNull()) {
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();
        if (jsonObj.value("error").toInt(ErrorCodes::ERR_JSON) != ErrorCodes::SUCCESS) {
            emit sig_user_search(nullptr);
            return;
        }

        auto apply_info = std::make_shared<AddFriendApply>(jsonObj["applyuid"].toInt(),
                                                           jsonObj["name"].toString(),
                                                           jsonObj["desc"].toString(),
                                                           jsonObj["icon"].toString(),
                                                           jsonObj["nick"].toString(),
                                                           jsonObj["sex"].toInt(),
                                                           jsonObj["user_id"].toString());
        emit sig_friend_apply(apply_info);
    });

    _handlers.insert(ID_NOTIFY_AUTH_FRIEND_REQ, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug() << "handle id is " << id << " data is " << data;

        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if (jsonDoc.isNull()) {
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();
        if (jsonObj.value("error").toInt(ErrorCodes::ERR_JSON) != ErrorCodes::SUCCESS) {
            return;
        }

        auto auth_info = std::make_shared<AuthInfo>(jsonObj["fromuid"].toInt(),
                                                    jsonObj["name"].toString(),
                                                    jsonObj["nick"].toString(),
                                                    jsonObj["icon"].toString(),
                                                    jsonObj["sex"].toInt(),
                                                    jsonObj["user_id"].toString());
        emit sig_add_auth_friend(auth_info);
    });

    _handlers.insert(ID_ADD_FRIEND_RSP, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(id);
        Q_UNUSED(len);
        Q_UNUSED(data);
    });

    _handlers.insert(ID_AUTH_FRIEND_RSP, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug() << "handle id is " << id << " data is " << data;

        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if (jsonDoc.isNull()) {
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();
        if (jsonObj.value("error").toInt(ErrorCodes::ERR_JSON) != ErrorCodes::SUCCESS) {
            return;
        }

        auto rsp = std::make_shared<AuthRsp>(jsonObj["uid"].toInt(),
                                             jsonObj["name"].toString(),
                                             jsonObj["nick"].toString(),
                                             jsonObj["icon"].toString(),
                                             jsonObj["sex"].toInt(),
                                             jsonObj["user_id"].toString());
        emit sig_auth_rsp(rsp);
    });

    _handlers.insert(ID_TEXT_CHAT_MSG_RSP, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(id);
        Q_UNUSED(len);
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if (jsonDoc.isNull()) {
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();
        if (!jsonObj.contains("error")) {
            return;
        }

        emit sig_message_status(jsonObj);
    });

    _handlers.insert(ID_NOTIFY_MSG_STATUS_REQ, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(id);
        Q_UNUSED(len);
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if (jsonDoc.isNull() || !jsonDoc.isObject()) {
            return;
        }
        emit sig_message_status(jsonDoc.object());
    });

    _handlers.insert(ID_NOTIFY_TEXT_CHAT_MSG_REQ, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug() << "handle id is " << id << " data is " << data;

        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if (jsonDoc.isNull()) {
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();
        if (jsonObj.value("error").toInt(ErrorCodes::ERR_JSON) != ErrorCodes::SUCCESS) {
            return;
        }

        auto msg_ptr = std::make_shared<TextChatMsg>(jsonObj["fromuid"].toInt(),
                                                     jsonObj["touid"].toInt(),
                                                     jsonObj["text_array"].toArray());
        emit sig_text_chat_msg(msg_ptr);
    });

    auto parse_group_rsp = [this](ReqId reqId, const QByteArray &data, QJsonObject &jsonObj) -> bool {
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if (jsonDoc.isNull() || !jsonDoc.isObject()) {
            emit sig_group_rsp(reqId, ErrorCodes::ERR_JSON, QJsonObject());
            return false;
        }

        jsonObj = jsonDoc.object();
        const int err = jsonObj.value("error").toInt(ErrorCodes::ERR_JSON);
        emit sig_group_rsp(reqId, err, jsonObj);
        return err == ErrorCodes::SUCCESS;
    };

    _handlers.insert(ID_CREATE_GROUP_RSP, [this, parse_group_rsp](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        QJsonObject jsonObj;
        if (!parse_group_rsp(id, data, jsonObj)) {
            return;
        }
        if (jsonObj.contains("group_list")) {
            UserMgr::GetInstance()->SetGroupList(jsonObj.value("group_list").toArray());
        }
        emit sig_group_list_updated();
    });

    _handlers.insert(ID_GET_GROUP_LIST_RSP, [this, parse_group_rsp](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        QJsonObject jsonObj;
        if (!parse_group_rsp(id, data, jsonObj)) {
            return;
        }
        if (jsonObj.contains("group_list")) {
            UserMgr::GetInstance()->SetGroupList(jsonObj.value("group_list").toArray());
        }
        emit sig_group_list_updated();
    });

    _handlers.insert(ID_GET_DIALOG_LIST_RSP, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        Q_UNUSED(id);
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if (jsonDoc.isNull() || !jsonDoc.isObject()) {
            QJsonObject fallback;
            fallback["error"] = ErrorCodes::ERR_JSON;
            emit sig_dialog_list_rsp(fallback);
            return;
        }
        emit sig_dialog_list_rsp(jsonDoc.object());
    });

    _handlers.insert(ID_INVITE_GROUP_MEMBER_RSP, [this, parse_group_rsp](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        QJsonObject jsonObj;
        parse_group_rsp(id, data, jsonObj);
    });
    _handlers.insert(ID_APPLY_JOIN_GROUP_RSP, [this, parse_group_rsp](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        QJsonObject jsonObj;
        parse_group_rsp(id, data, jsonObj);
    });
    _handlers.insert(ID_REVIEW_GROUP_APPLY_RSP, [this, parse_group_rsp](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        QJsonObject jsonObj;
        parse_group_rsp(id, data, jsonObj);
    });
    _handlers.insert(ID_GROUP_CHAT_MSG_RSP, [this, parse_group_rsp](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        QJsonObject jsonObj;
        parse_group_rsp(id, data, jsonObj);
    });

    auto normalize_group_created_at = [](qint64 createdAt) -> qint64 {
        if (createdAt <= 0) {
            return QDateTime::currentMSecsSinceEpoch();
        }
        if (createdAt < 100000000000LL) {
            return createdAt * 1000;
        }
        return createdAt;
    };

    _handlers.insert(ID_GROUP_HISTORY_RSP, [this, normalize_group_created_at](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if (jsonDoc.isNull() || !jsonDoc.isObject()) {
            emit sig_group_rsp(id, ErrorCodes::ERR_JSON, QJsonObject());
            return;
        }
        const QJsonObject jsonObj = jsonDoc.object();
        const int err = jsonObj.value("error").toInt(ErrorCodes::ERR_JSON);
        if (err != ErrorCodes::SUCCESS) {
            emit sig_group_rsp(id, err, jsonObj);
            return;
        }

        const qint64 groupId = jsonObj.value("groupid").toVariant().toLongLong();
        const QJsonArray messages = jsonObj.value("messages").toArray();
        for (int i = messages.size() - 1; i >= 0; --i) {
            const QJsonObject one = messages.at(i).toObject();
            const QString fromName = one.value("from_nick").toString(one.value("from_name").toString());
            const QString fromIcon = one.value("from_icon").toString();
            const qint64 createdAt = normalize_group_created_at(one.value("created_at").toVariant().toLongLong());
            const qint64 serverMsgId = one.value("server_msg_id").toVariant().toLongLong();
            const qint64 groupSeq = one.value("group_seq").toVariant().toLongLong();
            const qint64 replyToServerMsgId = one.value("reply_to_server_msg_id").toVariant().toLongLong();
            QString forwardMetaJson;
            const QJsonValue forwardMetaValue = one.value("forward_meta");
            if (forwardMetaValue.isObject()) {
                forwardMetaJson = QString::fromUtf8(QJsonDocument(forwardMetaValue.toObject()).toJson(QJsonDocument::Compact));
            } else if (forwardMetaValue.isArray()) {
                forwardMetaJson = QString::fromUtf8(QJsonDocument(forwardMetaValue.toArray()).toJson(QJsonDocument::Compact));
            } else if (forwardMetaValue.isString()) {
                forwardMetaJson = forwardMetaValue.toString();
            }
            const qint64 editedAtMs = one.value("edited_at_ms").toVariant().toLongLong();
            const qint64 deletedAtMs = one.value("deleted_at_ms").toVariant().toLongLong();
            QString state = QStringLiteral("sent");
            if (deletedAtMs > 0 || one.value("msgtype").toString() == QStringLiteral("revoke")
                || one.value("content").toString() == QStringLiteral("[消息已撤回]")) {
                state = QStringLiteral("deleted");
            } else if (editedAtMs > 0) {
                state = QStringLiteral("edited");
            }
            auto msg = std::make_shared<TextChatData>(one.value("msgid").toString(),
                                                      one.value("content").toString(),
                                                      one.value("fromuid").toInt(),
                                                      0,
                                                      fromName,
                                                      createdAt,
                                                      fromIcon,
                                                      state,
                                                      serverMsgId,
                                                      groupSeq,
                                                      replyToServerMsgId,
                                                      forwardMetaJson,
                                                      editedAtMs,
                                                      deletedAtMs);
            UserMgr::GetInstance()->UpsertGroupChatMsg(groupId, msg);
        }
        emit sig_group_rsp(id, err, jsonObj);
    });

    _handlers.insert(ID_UPDATE_GROUP_ANNOUNCEMENT_RSP, [this, parse_group_rsp](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        QJsonObject jsonObj;
        parse_group_rsp(id, data, jsonObj);
    });
    _handlers.insert(ID_MUTE_GROUP_MEMBER_RSP, [this, parse_group_rsp](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        QJsonObject jsonObj;
        parse_group_rsp(id, data, jsonObj);
    });
    _handlers.insert(ID_SET_GROUP_ADMIN_RSP, [this, parse_group_rsp](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        QJsonObject jsonObj;
        parse_group_rsp(id, data, jsonObj);
    });
    _handlers.insert(ID_UPDATE_GROUP_ICON_RSP, [this, parse_group_rsp](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        QJsonObject jsonObj;
        parse_group_rsp(id, data, jsonObj);
    });
    _handlers.insert(ID_SYNC_DRAFT_RSP, [this, parse_group_rsp](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        QJsonObject jsonObj;
        parse_group_rsp(id, data, jsonObj);
    });
    _handlers.insert(ID_PIN_DIALOG_RSP, [this, parse_group_rsp](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        QJsonObject jsonObj;
        parse_group_rsp(id, data, jsonObj);
    });
    _handlers.insert(ID_EDIT_GROUP_MSG_RSP, [this, parse_group_rsp](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        QJsonObject jsonObj;
        parse_group_rsp(id, data, jsonObj);
    });
    _handlers.insert(ID_REVOKE_GROUP_MSG_RSP, [this, parse_group_rsp](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        QJsonObject jsonObj;
        parse_group_rsp(id, data, jsonObj);
    });
    _handlers.insert(ID_EDIT_PRIVATE_MSG_RSP, [this, parse_group_rsp](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        QJsonObject jsonObj;
        parse_group_rsp(id, data, jsonObj);
    });
    _handlers.insert(ID_REVOKE_PRIVATE_MSG_RSP, [this, parse_group_rsp](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        QJsonObject jsonObj;
        parse_group_rsp(id, data, jsonObj);
    });
    _handlers.insert(ID_FORWARD_GROUP_MSG_RSP, [this, parse_group_rsp](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        QJsonObject jsonObj;
        parse_group_rsp(id, data, jsonObj);
    });
    _handlers.insert(ID_FORWARD_PRIVATE_MSG_RSP, [this, parse_group_rsp](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        QJsonObject jsonObj;
        parse_group_rsp(id, data, jsonObj);
    });
    _handlers.insert(ID_KICK_GROUP_MEMBER_RSP, [this, parse_group_rsp](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        QJsonObject jsonObj;
        parse_group_rsp(id, data, jsonObj);
    });
    _handlers.insert(ID_QUIT_GROUP_RSP, [this, parse_group_rsp](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        QJsonObject jsonObj;
        parse_group_rsp(id, data, jsonObj);
    });

    _handlers.insert(ID_PRIVATE_HISTORY_RSP, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        Q_UNUSED(id);
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if (jsonDoc.isNull() || !jsonDoc.isObject()) {
            QJsonObject fallback;
            fallback["error"] = ErrorCodes::ERR_JSON;
            emit sig_private_history_rsp(fallback);
            return;
        }
        emit sig_private_history_rsp(jsonDoc.object());
    });

    _handlers.insert(ID_NOTIFY_GROUP_INVITE_REQ, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(id);
        Q_UNUSED(len);
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if (jsonDoc.isNull() || !jsonDoc.isObject()) {
            return;
        }
        const QJsonObject jsonObj = jsonDoc.object();
        if (jsonObj.value("error").toInt(ErrorCodes::ERR_JSON) != ErrorCodes::SUCCESS) {
            return;
        }
        emit sig_group_invite(jsonObj.value("groupid").toVariant().toLongLong(),
                              jsonObj.value("group_code").toString(),
                              jsonObj.value("name").toString(),
                              jsonObj.value("operator_uid").toInt());
    });

    _handlers.insert(ID_NOTIFY_GROUP_APPLY_REQ, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(id);
        Q_UNUSED(len);
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if (jsonDoc.isNull() || !jsonDoc.isObject()) {
            return;
        }
        const QJsonObject jsonObj = jsonDoc.object();
        if (jsonObj.value("error").toInt(ErrorCodes::ERR_JSON) != ErrorCodes::SUCCESS) {
            return;
        }
        emit sig_group_apply(jsonObj.value("groupid").toVariant().toLongLong(),
                             jsonObj.value("applicant_uid").toInt(),
                             jsonObj.value("applicant_user_id").toString(),
                             jsonObj.value("reason").toString());
    });

    _handlers.insert(ID_NOTIFY_GROUP_MEMBER_CHANGED_REQ, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(id);
        Q_UNUSED(len);
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if (jsonDoc.isNull() || !jsonDoc.isObject()) {
            return;
        }
        const QJsonObject jsonObj = jsonDoc.object();
        if (jsonObj.value("error").toInt(ErrorCodes::ERR_JSON) != ErrorCodes::SUCCESS) {
            return;
        }
        emit sig_group_member_changed(jsonObj);
    });

    _handlers.insert(ID_NOTIFY_GROUP_CHAT_MSG_REQ, [this, normalize_group_created_at](ReqId id, int len, QByteArray data) {
        Q_UNUSED(id);
        Q_UNUSED(len);
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if (jsonDoc.isNull() || !jsonDoc.isObject()) {
            return;
        }
        const QJsonObject jsonObj = jsonDoc.object();
        if (jsonObj.value("error").toInt(ErrorCodes::ERR_JSON) != ErrorCodes::SUCCESS) {
            return;
        }
        const qint64 groupId = jsonObj.value("groupid").toVariant().toLongLong();
        const int fromUid = jsonObj.value("fromuid").toInt();
        QJsonObject msgObj = jsonObj.value("msg").toObject();
        const qint64 topCreatedAt = normalize_group_created_at(jsonObj.value("created_at").toVariant().toLongLong());
        const qint64 msgCreatedAt = msgObj.value("created_at").toVariant().toLongLong();
        msgObj["created_at"] = normalize_group_created_at(msgCreatedAt <= 0 ? topCreatedAt : msgCreatedAt);
        const qint64 topServerMsgId = jsonObj.value("server_msg_id").toVariant().toLongLong();
        if (!msgObj.contains("server_msg_id") || msgObj.value("server_msg_id").toVariant().toLongLong() <= 0) {
            msgObj["server_msg_id"] = topServerMsgId;
        }
        const qint64 topGroupSeq = jsonObj.value("group_seq").toVariant().toLongLong();
        if (!msgObj.contains("group_seq") || msgObj.value("group_seq").toVariant().toLongLong() <= 0) {
            msgObj["group_seq"] = topGroupSeq;
        }
        if (!msgObj.contains("reply_to_server_msg_id")) {
            const qint64 topReplyToServerMsgId = jsonObj.value("reply_to_server_msg_id").toVariant().toLongLong();
            if (topReplyToServerMsgId > 0) {
                msgObj["reply_to_server_msg_id"] = topReplyToServerMsgId;
            }
        }
        if (!msgObj.contains("forward_meta") && jsonObj.contains("forward_meta")) {
            msgObj["forward_meta"] = jsonObj.value("forward_meta");
        }
        if (!msgObj.contains("edited_at_ms") && jsonObj.contains("edited_at_ms")) {
            msgObj["edited_at_ms"] = jsonObj.value("edited_at_ms");
        }
        if (!msgObj.contains("deleted_at_ms") && jsonObj.contains("deleted_at_ms")) {
            msgObj["deleted_at_ms"] = jsonObj.value("deleted_at_ms");
        }
        const QString fromName = jsonObj.value("from_nick").toString(jsonObj.value("from_name").toString());
        const QString fromIcon = jsonObj.value("from_icon").toString();
        auto groupMsg = std::make_shared<GroupChatMsg>(groupId, fromUid, msgObj, fromName, fromIcon);
        emit sig_group_chat_msg(groupMsg);
    });

    _handlers.insert(ID_NOTIFY_PRIVATE_MSG_CHANGED_REQ, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(id);
        Q_UNUSED(len);
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if (jsonDoc.isNull() || !jsonDoc.isObject()) {
            return;
        }
        const QJsonObject jsonObj = jsonDoc.object();
        if (jsonObj.value("error").toInt(ErrorCodes::ERR_JSON) != ErrorCodes::SUCCESS) {
            return;
        }
        emit sig_private_msg_changed(jsonObj);
    });

    _handlers.insert(ID_NOTIFY_PRIVATE_READ_ACK_REQ, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(id);
        Q_UNUSED(len);
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if (jsonDoc.isNull() || !jsonDoc.isObject()) {
            return;
        }
        const QJsonObject jsonObj = jsonDoc.object();
        if (jsonObj.value("error").toInt(ErrorCodes::ERR_JSON) != ErrorCodes::SUCCESS) {
            return;
        }
        emit sig_private_read_ack(jsonObj);
    });

    _handlers.insert(ID_NOTIFY_CALL_EVENT_REQ, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(id);
        Q_UNUSED(len);
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if (jsonDoc.isNull() || !jsonDoc.isObject()) {
            qWarning() << "call event parse failed";
            return;
        }
        const QJsonObject jsonObj = jsonDoc.object();
        if (jsonObj.value("error").toInt(ErrorCodes::SUCCESS) != ErrorCodes::SUCCESS) {
            return;
        }
        emit sig_call_event(jsonObj);
    });

    _handlers.insert(ID_NOTIFY_OFF_LINE_REQ, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(id);
        Q_UNUSED(len);
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if (jsonDoc.isNull()) {
            return;
        }
        const QJsonObject jsonObj = jsonDoc.object();
        if (jsonObj.value("error").toInt(ErrorCodes::ERR_JSON) != ErrorCodes::SUCCESS) {
            return;
        }
        emit sig_notify_offline();
    });

    _handlers.insert(ID_HEARTBEAT_RSP, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(id);
        Q_UNUSED(len);
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if (jsonDoc.isNull()) {
            return;
        }
        const QJsonObject jsonObj = jsonDoc.object();
        if (jsonObj.value("error").toInt(ErrorCodes::ERR_JSON) != ErrorCodes::SUCCESS) {
            return;
        }
        emit sig_heartbeat_ack(QDateTime::currentMSecsSinceEpoch());
    });
}

void ChatMessageDispatcher::dispatchMessage(ReqId id, int len, const QByteArray &data)
{
    auto find_iter = _handlers.find(id);
    if (find_iter == _handlers.end()) {
        qDebug() << "not found id [" << id << "] to handle";
        return;
    }

    find_iter.value()(id, len, data);
}
