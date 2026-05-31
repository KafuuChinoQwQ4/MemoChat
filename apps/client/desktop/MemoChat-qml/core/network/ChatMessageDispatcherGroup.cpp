#include "ChatMessageDispatcher.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "ChatMessageDispatcherGroupHistory.h"
#include "ChatMessageDispatcherGroupPayload.h"
#include "usermgr.h"

bool ChatMessageDispatcher::parseGroupResponse(ReqId reqId,
                                               const QByteArray& data,
                                               QJsonObject& jsonObj,
                                               bool emitResponse)
{
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
    if (jsonDoc.isNull() || !jsonDoc.isObject())
    {
        if (emitResponse)
        {
            emit sig_group_rsp(reqId, ErrorCodes::ERR_JSON, QJsonObject());
        }
        return false;
    }

    jsonObj = jsonDoc.object();
    const int err = jsonObj.value("error").toInt(ErrorCodes::ERR_JSON);
    if (emitResponse)
    {
        emit sig_group_rsp(reqId, err, jsonObj);
    }
    return err == ErrorCodes::SUCCESS;
}

void ChatMessageDispatcher::registerGroupHandlers()
{
    _handlers.insert(ID_CREATE_GROUP_RSP,
                     [this](ReqId id, int len, QByteArray data)
                     {
                         Q_UNUSED(len);
                         QJsonObject jsonObj;
                         if (!parseGroupResponse(id, data, jsonObj, false))
                         {
                             Q_EMIT
                             sig_group_rsp(id, jsonObj.value("error").toInt(ErrorCodes::ERR_JSON), jsonObj);
                             return;
                         }
                         if (jsonObj.contains("group_list"))
                         {
                             UserMgr::GetInstance()->SetGroupList(jsonObj.value("group_list").toArray());
                         }
                         emit sig_group_rsp(id, jsonObj.value("error").toInt(ErrorCodes::ERR_JSON), jsonObj);
                         emit sig_group_list_updated();
                     });

    _handlers.insert(ID_GET_GROUP_LIST_RSP,
                     [this](ReqId id, int len, QByteArray data)
                     {
                         Q_UNUSED(len);
                         QJsonObject jsonObj;
                         if (!parseGroupResponse(id, data, jsonObj, false))
                         {
                             Q_EMIT
                             sig_group_rsp(id, jsonObj.value("error").toInt(ErrorCodes::ERR_JSON), jsonObj);
                             return;
                         }
                         if (jsonObj.contains("group_list"))
                         {
                             UserMgr::GetInstance()->SetGroupList(jsonObj.value("group_list").toArray());
                         }
                         emit sig_group_rsp(id, jsonObj.value("error").toInt(ErrorCodes::ERR_JSON), jsonObj);
                         emit sig_group_list_updated();
                     });

    _handlers.insert(ID_INVITE_GROUP_MEMBER_RSP,
                     [this](ReqId id, int len, QByteArray data)
                     {
                         Q_UNUSED(len);
                         QJsonObject jsonObj;
                         parseGroupResponse(id, data, jsonObj);
                     });
    _handlers.insert(ID_APPLY_JOIN_GROUP_RSP,
                     [this](ReqId id, int len, QByteArray data)
                     {
                         Q_UNUSED(len);
                         QJsonObject jsonObj;
                         parseGroupResponse(id, data, jsonObj);
                     });
    _handlers.insert(ID_REVIEW_GROUP_APPLY_RSP,
                     [this](ReqId id, int len, QByteArray data)
                     {
                         Q_UNUSED(len);
                         QJsonObject jsonObj;
                         parseGroupResponse(id, data, jsonObj);
                     });
    _handlers.insert(ID_GROUP_CHAT_MSG_RSP,
                     [this](ReqId id, int len, QByteArray data)
                     {
                         Q_UNUSED(len);
                         QJsonObject jsonObj;
                         parseGroupResponse(id, data, jsonObj);
                     });

    _handlers.insert(ID_GROUP_HISTORY_RSP,
                     [this](ReqId id, int len, QByteArray data)
                     {
                         Q_UNUSED(len);
                         QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
                         if (jsonDoc.isNull() || !jsonDoc.isObject())
                         {
                             emit sig_group_rsp(id, ErrorCodes::ERR_JSON, QJsonObject());
                             return;
                         }
                         const QJsonObject jsonObj = jsonDoc.object();
                         const int err = jsonObj.value("error").toInt(ErrorCodes::ERR_JSON);
                         if (err != ErrorCodes::SUCCESS)
                         {
                             emit sig_group_rsp(id, err, jsonObj);
                             return;
                         }

                         const qint64 groupId = jsonObj.value("groupid").toVariant().toLongLong();
                         const QJsonArray messages = jsonObj.value("messages").toArray();
                         for (int i = messages.size() - 1; i >= 0; --i)
                         {
                             const QJsonObject one = messages.at(i).toObject();
                             const auto msg = ChatMessageDispatcherGroupHistory::buildHistoryTextMessage(one);
                             UserMgr::GetInstance()->UpsertGroupChatMsg(groupId, msg);
                         }
                         emit sig_group_rsp(id, err, jsonObj);
                     });

    _handlers.insert(ID_UPDATE_GROUP_ANNOUNCEMENT_RSP,
                     [this](ReqId id, int len, QByteArray data)
                     {
                         Q_UNUSED(len);
                         QJsonObject jsonObj;
                         parseGroupResponse(id, data, jsonObj);
                     });
    _handlers.insert(ID_MUTE_GROUP_MEMBER_RSP,
                     [this](ReqId id, int len, QByteArray data)
                     {
                         Q_UNUSED(len);
                         QJsonObject jsonObj;
                         parseGroupResponse(id, data, jsonObj);
                     });
    _handlers.insert(ID_SET_GROUP_ADMIN_RSP,
                     [this](ReqId id, int len, QByteArray data)
                     {
                         Q_UNUSED(len);
                         QJsonObject jsonObj;
                         parseGroupResponse(id, data, jsonObj);
                     });
    _handlers.insert(ID_UPDATE_GROUP_ICON_RSP,
                     [this](ReqId id, int len, QByteArray data)
                     {
                         Q_UNUSED(len);
                         QJsonObject jsonObj;
                         parseGroupResponse(id, data, jsonObj);
                     });
    _handlers.insert(ID_EDIT_GROUP_MSG_RSP,
                     [this](ReqId id, int len, QByteArray data)
                     {
                         Q_UNUSED(len);
                         QJsonObject jsonObj;
                         parseGroupResponse(id, data, jsonObj);
                     });
    _handlers.insert(ID_REVOKE_GROUP_MSG_RSP,
                     [this](ReqId id, int len, QByteArray data)
                     {
                         Q_UNUSED(len);
                         QJsonObject jsonObj;
                         parseGroupResponse(id, data, jsonObj);
                     });
    _handlers.insert(ID_FORWARD_GROUP_MSG_RSP,
                     [this](ReqId id, int len, QByteArray data)
                     {
                         Q_UNUSED(len);
                         QJsonObject jsonObj;
                         parseGroupResponse(id, data, jsonObj);
                     });
    _handlers.insert(ID_KICK_GROUP_MEMBER_RSP,
                     [this](ReqId id, int len, QByteArray data)
                     {
                         Q_UNUSED(len);
                         QJsonObject jsonObj;
                         parseGroupResponse(id, data, jsonObj);
                     });
    _handlers.insert(ID_QUIT_GROUP_RSP,
                     [this](ReqId id, int len, QByteArray data)
                     {
                         Q_UNUSED(len);
                         QJsonObject jsonObj;
                         parseGroupResponse(id, data, jsonObj);
                     });
    _handlers.insert(ID_DISSOLVE_GROUP_RSP,
                     [this](ReqId id, int len, QByteArray data)
                     {
                         Q_UNUSED(len);
                         QJsonObject jsonObj;
                         parseGroupResponse(id, data, jsonObj);
                     });

    _handlers.insert(ID_NOTIFY_GROUP_INVITE_REQ,
                     [this](ReqId id, int len, QByteArray data)
                     {
                         Q_UNUSED(id);
                         Q_UNUSED(len);
                         QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
                         if (jsonDoc.isNull() || !jsonDoc.isObject())
                         {
                             return;
                         }
                         const QJsonObject jsonObj = jsonDoc.object();
                         if (jsonObj.value("error").toInt(ErrorCodes::ERR_JSON) != ErrorCodes::SUCCESS)
                         {
                             return;
                         }
                         emit sig_group_invite(jsonObj.value("groupid").toVariant().toLongLong(),
                                               jsonObj.value("group_code").toString(),
                                               jsonObj.value("name").toString(),
                                               jsonObj.value("operator_uid").toInt());
                     });

    _handlers.insert(ID_NOTIFY_GROUP_APPLY_REQ,
                     [this](ReqId id, int len, QByteArray data)
                     {
                         Q_UNUSED(id);
                         Q_UNUSED(len);
                         QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
                         if (jsonDoc.isNull() || !jsonDoc.isObject())
                         {
                             return;
                         }
                         const QJsonObject jsonObj = jsonDoc.object();
                         if (jsonObj.value("error").toInt(ErrorCodes::ERR_JSON) != ErrorCodes::SUCCESS)
                         {
                             return;
                         }
                         emit sig_group_apply(jsonObj.value("groupid").toVariant().toLongLong(),
                                              jsonObj.value("applicant_uid").toInt(),
                                              jsonObj.value("applicant_user_id").toString(),
                                              jsonObj.value("reason").toString());
                     });

    _handlers.insert(ID_NOTIFY_GROUP_MEMBER_CHANGED_REQ,
                     [this](ReqId id, int len, QByteArray data)
                     {
                         Q_UNUSED(id);
                         Q_UNUSED(len);
                         QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
                         if (jsonDoc.isNull() || !jsonDoc.isObject())
                         {
                             return;
                         }
                         const QJsonObject jsonObj = jsonDoc.object();
                         if (jsonObj.value("error").toInt(ErrorCodes::ERR_JSON) != ErrorCodes::SUCCESS)
                         {
                             return;
                         }
                         emit sig_group_member_changed(jsonObj);
                     });

    _handlers.insert(ID_NOTIFY_GROUP_CHAT_MSG_REQ,
                     [this](ReqId id, int len, QByteArray data)
                     {
                         Q_UNUSED(id);
                         Q_UNUSED(len);
                         QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
                         if (jsonDoc.isNull() || !jsonDoc.isObject())
                         {
                             return;
                         }
                         const QJsonObject jsonObj = jsonDoc.object();
                         if (jsonObj.value("error").toInt(ErrorCodes::ERR_JSON) != ErrorCodes::SUCCESS)
                         {
                             return;
                         }
                         const qint64 groupId = jsonObj.value("groupid").toVariant().toLongLong();
                         const int fromUid = jsonObj.value("fromuid").toInt();
                         const QJsonObject msgObj = ChatMessageDispatcherGroupPayload::normalizedNotifyMessage(jsonObj);
                         const QString fromName =
                             jsonObj.value("from_nick").toString(jsonObj.value("from_name").toString());
                         const QString fromIcon = jsonObj.value("from_icon").toString();
                         auto groupMsg = std::make_shared<GroupChatMsg>(groupId, fromUid, msgObj, fromName, fromIcon);
                         emit sig_group_chat_msg(groupMsg);
                     });
}
