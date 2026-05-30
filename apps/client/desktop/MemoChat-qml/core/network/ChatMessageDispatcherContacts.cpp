#include "ChatMessageDispatcher.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

#include "usermgr.h"

void ChatMessageDispatcher::registerContactHandlers()
{
    _handlers.insert(ID_GET_RELATION_BOOTSTRAP_RSP,
                     [this](ReqId id, int len, QByteArray data)
                     {
                         Q_UNUSED(id);
                         Q_UNUSED(len);
                         QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
                         if (jsonDoc.isNull() || !jsonDoc.isObject())
                         {
                             qWarning() << "relation bootstrap rsp json parse failed";
                             return;
                         }

                         const QJsonObject jsonObj = jsonDoc.object();
                         const int err = jsonObj.value("error").toInt(ErrorCodes::ERR_JSON);
                         if (err != ErrorCodes::SUCCESS)
                         {
                             qWarning() << "relation bootstrap rsp failed, err:" << err;
                             return;
                         }

                         if (jsonObj.contains("apply_list"))
                         {
                             UserMgr::GetInstance()->AppendApplyList(jsonObj.value("apply_list").toArray());
                         }
                         if (jsonObj.contains("friend_list"))
                         {
                             UserMgr::GetInstance()->AppendFriendList(jsonObj.value("friend_list").toArray());
                         }

                         emit sig_relation_bootstrap_updated();
                     });

    _handlers.insert(ID_SEARCH_USER_RSP,
                     [this](ReqId id, int len, QByteArray data)
                     {
                         Q_UNUSED(len);
                         qDebug() << "handle id is " << id << " data is " << data;

                         QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
                         if (jsonDoc.isNull())
                         {
                             return;
                         }

                         QJsonObject jsonObj = jsonDoc.object();
                         if (jsonObj.value("error").toInt(ErrorCodes::ERR_JSON) != ErrorCodes::SUCCESS)
                         {
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

    _handlers.insert(ID_NOTIFY_ADD_FRIEND_REQ,
                     [this](ReqId id, int len, QByteArray data)
                     {
                         Q_UNUSED(len);
                         qDebug() << "handle id is " << id << " data is " << data;

                         QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
                         if (jsonDoc.isNull())
                         {
                             return;
                         }

                         QJsonObject jsonObj = jsonDoc.object();
                         if (jsonObj.value("error").toInt(ErrorCodes::ERR_JSON) != ErrorCodes::SUCCESS)
                         {
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

    _handlers.insert(ID_NOTIFY_AUTH_FRIEND_REQ,
                     [this](ReqId id, int len, QByteArray data)
                     {
                         Q_UNUSED(len);
                         qDebug() << "handle id is " << id << " data is " << data;

                         QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
                         if (jsonDoc.isNull())
                         {
                             return;
                         }

                         QJsonObject jsonObj = jsonDoc.object();
                         if (jsonObj.value("error").toInt(ErrorCodes::ERR_JSON) != ErrorCodes::SUCCESS)
                         {
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

    _handlers.insert(ID_ADD_FRIEND_RSP,
                     [this](ReqId id, int len, QByteArray data)
                     {
                         Q_UNUSED(id);
                         Q_UNUSED(len);
                         Q_UNUSED(data);
                     });

    _handlers.insert(ID_AUTH_FRIEND_RSP,
                     [this](ReqId id, int len, QByteArray data)
                     {
                         Q_UNUSED(len);
                         qDebug() << "handle id is " << id << " data is " << data;

                         QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
                         if (jsonDoc.isNull())
                         {
                             return;
                         }

                         QJsonObject jsonObj = jsonDoc.object();
                         if (jsonObj.value("error").toInt(ErrorCodes::ERR_JSON) != ErrorCodes::SUCCESS)
                         {
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

    _handlers.insert(ID_DELETE_FRIEND_RSP,
                     [this](ReqId id, int len, QByteArray data)
                     {
                         Q_UNUSED(id);
                         Q_UNUSED(len);
                         QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
                         if (jsonDoc.isNull() || !jsonDoc.isObject())
                         {
                             emit sig_delete_friend_rsp(ErrorCodes::ERR_JSON, 0);
                             return;
                         }

                         const QJsonObject jsonObj = jsonDoc.object();
                         emit sig_delete_friend_rsp(jsonObj.value("error").toInt(ErrorCodes::ERR_JSON),
                                                    jsonObj.value("friend_uid").toInt());
                     });
}
