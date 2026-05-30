#include "ChatMessageDispatcher.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

#include "usermgr.h"

void ChatMessageDispatcher::registerAuthHandlers()
{
    _handlers.insert(ID_CHAT_LOGIN_RSP,
                     [this](ReqId id, int len, QByteArray data)
                     {
                         Q_UNUSED(len);
                         qDebug() << "handle id is " << id;

                         QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
                         if (jsonDoc.isNull())
                         {
                             qWarning() << "chat login rsp json parse failed";
                             emit sig_login_failed(ErrorCodes::ERR_JSON);
                             return;
                         }

                         QJsonObject jsonObj = jsonDoc.object();
                         qDebug() << "data jsonobj is " << jsonObj;

                         if (!jsonObj.contains("error"))
                         {
                             emit sig_login_failed(ErrorCodes::ERR_JSON);
                             return;
                         }

                         const int err = jsonObj["error"].toInt();
                         if (err != ErrorCodes::SUCCESS)
                         {
                             qWarning() << "chat login rsp failed, err:" << err;
                             emit sig_login_failed(err);
                             return;
                         }

                         auto user_info = UserMgr::GetInstance()->GetUserInfo();
                         const QString responseIcon = jsonObj["icon"].toString();
                         if (!user_info)
                         {
                             user_info = std::make_shared<UserInfo>(jsonObj["uid"].toInt(),
                                                                    jsonObj["name"].toString(),
                                                                    jsonObj["nick"].toString(),
                                                                    responseIcon,
                                                                    jsonObj["sex"].toInt(),
                                                                    "",
                                                                    jsonObj["desc"].toString(),
                                                                    jsonObj["user_id"].toString());
                             UserMgr::GetInstance()->SetUserInfo(user_info);
                         }
                         else
                         {
                             user_info->_uid = jsonObj["uid"].toInt(user_info->_uid);
                             user_info->_name = jsonObj["name"].toString(user_info->_name);
                             user_info->_nick = jsonObj["nick"].toString(user_info->_nick);
                             if (!responseIcon.trimmed().isEmpty())
                             {
                                 user_info->_icon = responseIcon;
                             }
                             user_info->_sex = jsonObj["sex"].toInt(user_info->_sex);
                             user_info->_desc = jsonObj["desc"].toString(user_info->_desc);
                             user_info->_user_id = jsonObj["user_id"].toString(user_info->_user_id);
                         }
                         const QString responseToken = jsonObj["token"].toString();
                         if (!responseToken.isEmpty())
                         {
                             UserMgr::GetInstance()->SetToken(responseToken);
                         }
                         if (jsonObj.contains("apply_list"))
                         {
                             UserMgr::GetInstance()->AppendApplyList(jsonObj["apply_list"].toArray());
                         }
                         if (jsonObj.contains("friend_list"))
                         {
                             UserMgr::GetInstance()->AppendFriendList(jsonObj["friend_list"].toArray());
                         }

                         emit sig_swich_chatdlg();
                     });
}
