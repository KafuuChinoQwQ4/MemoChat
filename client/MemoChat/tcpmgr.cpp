#include "tcpmgr.h"
#include <limits>
#include <QAbstractSocket>
#include <QDateTime>
#include <QtEndian>
#include "usermgr.h"

TcpMgr::TcpMgr():_host(""),_port(0),_b_recv_pending(false),_connecting(false),_message_id(0),_message_len(0)
{
    _connect_timeout_timer.setSingleShot(true);
    _connect_timeout_timer.setInterval(8000);
    QObject::connect(&_connect_timeout_timer, &QTimer::timeout, [this]() {
        if (!_connecting) {
            return;
        }
        qWarning() << "TCP connect timeout. host:" << _host << "port:" << _port;
        _connecting = false;
        _socket.abort();
        emit sig_con_success(false);
    });

    QObject::connect(&_socket, &QTcpSocket::connected, [&]() {
           qDebug() << "Connected to server!";
           _connect_timeout_timer.stop();
           _connecting = false;
           _buffer.clear();
           _b_recv_pending = false;
           _message_id = 0;
           _message_len = 0;

            emit sig_con_success(true);
       });

       QObject::connect(&_socket, &QTcpSocket::readyRead, [&]() {
           static constexpr int kHeaderLen = sizeof(quint16) * 2;
           static constexpr quint16 kMaxBodyLen = std::numeric_limits<quint16>::max();

           _buffer.append(_socket.readAll());

           while (true) {
               if (!_b_recv_pending) {
                   if (_buffer.size() < kHeaderLen) {
                       break;
                   }

                   const auto* raw = reinterpret_cast<const uchar*>(_buffer.constData());
                   _message_id = qFromBigEndian<quint16>(raw);
                   _message_len = qFromBigEndian<quint16>(raw + sizeof(quint16));
                   _buffer.remove(0, kHeaderLen);

                   if (_message_len > kMaxBodyLen) {
                       qWarning() << "invalid message length:" << _message_len << ", reset parser";
                       _buffer.clear();
                       _b_recv_pending = false;
                       _message_id = 0;
                       _message_len = 0;
                       break;
                   }

                   qDebug() << "Message ID:" << _message_id << ", Length:" << _message_len;
                   _b_recv_pending = true;
               }

               if (_buffer.size() < _message_len) {
                   break;
               }

               QByteArray messageBody = _buffer.left(_message_len);
               _buffer.remove(0, _message_len);
               _b_recv_pending = false;

               qDebug() << "receive body msg is " << messageBody;
               handleMsg(ReqId(_message_id), _message_len, messageBody);
           }
       });


//       QObject::connect(&_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred), [&](QAbstractSocket::SocketError socketError) {
//           Q_UNUSED(socketError)
//           qDebug() << "Error:" << _socket.errorString();
//       });


        QObject::connect(&_socket, &QTcpSocket::errorOccurred,
                            [this](QAbstractSocket::SocketError socketError) {
               qDebug() << "Error:" << _socket.errorString() ;
               const bool wasConnecting = _connecting;
               if (wasConnecting) {
                   _connecting = false;
                   _connect_timeout_timer.stop();
               }
               switch (socketError) {
                   case QTcpSocket::ConnectionRefusedError:
                       qDebug() << "Connection Refused!";
                       if (wasConnecting) {
                           emit sig_con_success(false);
                       }
                       break;
                   case QTcpSocket::RemoteHostClosedError:
                       qDebug() << "Remote Host Closed Connection!";
                       if (wasConnecting) {
                           emit sig_con_success(false);
                       }
                       break;
                   case QTcpSocket::HostNotFoundError:
                       qDebug() << "Host Not Found!";
                       if (wasConnecting) {
                           emit sig_con_success(false);
                       }
                       break;
                   case QTcpSocket::SocketTimeoutError:
                       qDebug() << "Connection Timeout!";
                       if (wasConnecting) {
                           emit sig_con_success(false);
                       }
                       break;
                   case QTcpSocket::NetworkError:
                       qDebug() << "Network Error!";
                       if (wasConnecting) {
                           emit sig_con_success(false);
                       }
                       break;
                   default:
                       qDebug() << "Other Error!";
                       if (wasConnecting) {
                           emit sig_con_success(false);
                       }
                       break;
               }
         });


        QObject::connect(&_socket, &QTcpSocket::disconnected, [&]() {
            qDebug() << "Disconnected from server.";
            if (_connecting) {
                _connecting = false;
                _connect_timeout_timer.stop();
                emit sig_con_success(false);
            }
            _buffer.clear();
            _b_recv_pending = false;
            _message_id = 0;
            _message_len = 0;

            emit sig_connection_closed();
        });

        QObject::connect(this, &TcpMgr::sig_send_data, this, &TcpMgr::slot_send_data);

        initHandlers();
}

void TcpMgr::CloseConnection(){
    _connect_timeout_timer.stop();
    _connecting = false;
    _socket.close();
}

TcpMgr::~TcpMgr(){

}
void TcpMgr::initHandlers()
{
    //auto self = shared_from_this();
    _handlers.insert(ID_CHAT_LOGIN_RSP, [this](ReqId id, int len, QByteArray data){
        Q_UNUSED(len);
        qDebug()<< "handle id is "<< id ;

        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);


        if(jsonDoc.isNull()){
           qDebug() << "Failed to create QJsonDocument.";
           return;
        }

        QJsonObject jsonObj = jsonDoc.object();
        qDebug()<< "data jsonobj is " << jsonObj ;

        if(!jsonObj.contains("error")){
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "Login Failed, err is Json Parse Err" << err ;
            emit sig_login_failed(err);
            return;
        }

        int err = jsonObj["error"].toInt();
        if(err != ErrorCodes::SUCCESS){
            qDebug() << "Login Failed, err is " << err ;
            emit sig_login_failed(err);
            return;
        }
        
        auto uid = jsonObj["uid"].toInt();
        auto name = jsonObj["name"].toString();
        auto nick = jsonObj["nick"].toString();
        auto icon = jsonObj["icon"].toString();
        auto sex = jsonObj["sex"].toInt();
        auto desc = jsonObj["desc"].toString();
        auto userId = jsonObj["user_id"].toString();
        auto user_info = std::make_shared<UserInfo>(uid, name, nick, icon, sex,"",desc, userId);
 
        UserMgr::GetInstance()->SetUserInfo(user_info);
        UserMgr::GetInstance()->SetToken(jsonObj["token"].toString());
        if(jsonObj.contains("apply_list")){
            UserMgr::GetInstance()->AppendApplyList(jsonObj["apply_list"].toArray());
        }


        if (jsonObj.contains("friend_list")) {
            UserMgr::GetInstance()->AppendFriendList(jsonObj["friend_list"].toArray());
        }

        emit sig_swich_chatdlg();
    });


	_handlers.insert(ID_SEARCH_USER_RSP, [this](ReqId id, int len, QByteArray data) {
		Q_UNUSED(len);
		qDebug() << "handle id is " << id << " data is " << data;

		QJsonDocument jsonDoc = QJsonDocument::fromJson(data);


		if (jsonDoc.isNull()) {
			qDebug() << "Failed to create QJsonDocument.";
			return;
		}

		QJsonObject jsonObj = jsonDoc.object();

		if (!jsonObj.contains("error")) {
			int err = ErrorCodes::ERR_JSON;
			qDebug() << "Login Failed, err is Json Parse Err" << err;

			emit sig_user_search(nullptr);
			return;
		}

		int err = jsonObj["error"].toInt();
		if (err != ErrorCodes::SUCCESS) {
			qDebug() << "Login Failed, err is " << err;
            emit sig_user_search(nullptr);
			return;
		}
       auto search_info =  std::make_shared<SearchInfo>(jsonObj["uid"].toInt(), jsonObj["name"].toString(),
            jsonObj["nick"].toString(), jsonObj["desc"].toString(),
               jsonObj["sex"].toInt(), jsonObj["icon"].toString(), jsonObj["user_id"].toString());

        emit sig_user_search(search_info);
		});

	_handlers.insert(ID_NOTIFY_ADD_FRIEND_REQ, [this](ReqId id, int len, QByteArray data) {
		Q_UNUSED(len);
		qDebug() << "handle id is " << id << " data is " << data;

		QJsonDocument jsonDoc = QJsonDocument::fromJson(data);


		if (jsonDoc.isNull()) {
			qDebug() << "Failed to create QJsonDocument.";
			return;
		}

		QJsonObject jsonObj = jsonDoc.object();

		if (!jsonObj.contains("error")) {
			int err = ErrorCodes::ERR_JSON;
			qDebug() << "Login Failed, err is Json Parse Err" << err;

			emit sig_user_search(nullptr);
			return;
		}

		int err = jsonObj["error"].toInt();
		if (err != ErrorCodes::SUCCESS) {
			qDebug() << "Login Failed, err is " << err;
			emit sig_user_search(nullptr);
			return;
		}

         int from_uid = jsonObj["applyuid"].toInt();
         QString name = jsonObj["name"].toString();
         QString desc = jsonObj["desc"].toString();
         QString icon = jsonObj["icon"].toString();
         QString nick = jsonObj["nick"].toString();
         int sex = jsonObj["sex"].toInt();
         QString userId = jsonObj["user_id"].toString();

        auto apply_info = std::make_shared<AddFriendApply>(
                    from_uid, name, desc,
                      icon, nick, sex, userId);

		emit sig_friend_apply(apply_info);
		});

    _handlers.insert(ID_NOTIFY_AUTH_FRIEND_REQ, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug() << "handle id is " << id << " data is " << data;

        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);


        if (jsonDoc.isNull()) {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();
        if (!jsonObj.contains("error")) {
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "Auth Friend Failed, err is " << err;
            return;
        }

        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS) {
            qDebug() << "Auth Friend Failed, err is " << err;
            return;
        }

        int from_uid = jsonObj["fromuid"].toInt();
        QString name = jsonObj["name"].toString();
        QString nick = jsonObj["nick"].toString();
        QString icon = jsonObj["icon"].toString();
        int sex = jsonObj["sex"].toInt();
        QString userId = jsonObj["user_id"].toString();

        auto auth_info = std::make_shared<AuthInfo>(from_uid,name,
                                                    nick, icon, sex, userId);

        emit sig_add_auth_friend(auth_info);
        });

    _handlers.insert(ID_ADD_FRIEND_RSP, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug() << "handle id is " << id << " data is " << data;

        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);


        if (jsonDoc.isNull()) {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();

        if (!jsonObj.contains("error")) {
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "Add Friend Failed, err is Json Parse Err" << err;
            return;
        }

        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS) {
            qDebug() << "Add Friend Failed, err is " << err;
            return;
        }

         qDebug() << "Add Friend Success " ;
      });


    _handlers.insert(ID_AUTH_FRIEND_RSP, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug() << "handle id is " << id << " data is " << data;

        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);


        if (jsonDoc.isNull()) {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();

        if (!jsonObj.contains("error")) {
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "Auth Friend Failed, err is Json Parse Err" << err;
            return;
        }

        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS) {
            qDebug() << "Auth Friend Failed, err is " << err;
            return;
        }

        auto name = jsonObj["name"].toString();
        auto nick = jsonObj["nick"].toString();
        auto icon = jsonObj["icon"].toString();
        auto sex = jsonObj["sex"].toInt();
        auto uid = jsonObj["uid"].toInt();
        auto userId = jsonObj["user_id"].toString();
        auto rsp = std::make_shared<AuthRsp>(uid, name, nick, icon, sex, userId);
        emit sig_auth_rsp(rsp);

        qDebug() << "Auth Friend Success " ;
      });


    _handlers.insert(ID_TEXT_CHAT_MSG_RSP, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug() << "handle id is " << id << " data is " << data;

        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);


        if (jsonDoc.isNull()) {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();

        if (!jsonObj.contains("error")) {
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "Chat Msg Rsp Failed, err is Json Parse Err" << err;
            return;
        }

        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS) {
            qDebug() << "Chat Msg Rsp Failed, err is " << err;
            return;
        }

        qDebug() << "Receive Text Chat Rsp Success " ;

      });

    _handlers.insert(ID_NOTIFY_TEXT_CHAT_MSG_REQ, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug() << "handle id is " << id << " data is " << data;

        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);


        if (jsonDoc.isNull()) {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();

        if (!jsonObj.contains("error")) {
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "Notify Chat Msg Failed, err is Json Parse Err" << err;
            return;
        }

        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS) {
            qDebug() << "Notify Chat Msg Failed, err is " << err;
            return;
        }

        qDebug() << "Receive Text Chat Notify Success " ;
        auto msg_ptr = std::make_shared<TextChatMsg>(jsonObj["fromuid"].toInt(),
                jsonObj["touid"].toInt(),jsonObj["text_array"].toArray());
        emit sig_text_chat_msg(msg_ptr);
      });

    auto parse_group_rsp = [this](ReqId reqId, const QByteArray &data, QJsonObject &jsonObj) -> bool {
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if (jsonDoc.isNull() || !jsonDoc.isObject()) {
            qDebug() << "Group rsp parse failed, req id is " << reqId;
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
        QJsonObject jsonObj = jsonDoc.object();
        emit sig_private_history_rsp(jsonObj);
      });

    _handlers.insert(ID_NOTIFY_GROUP_INVITE_REQ, [this](ReqId id, int len, QByteArray data) {
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

    _handlers.insert(ID_NOTIFY_OFF_LINE_REQ,[this](ReqId id, int len, QByteArray data){
        Q_UNUSED(len);
        qDebug() << "handle id is " << id << " data is " << data;

        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);


        if (jsonDoc.isNull()) {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();

        if (!jsonObj.contains("error")) {
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "Notify Chat Msg Failed, err is Json Parse Err" << err;
            return;
        }

        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS) {
            qDebug() << "Notify Chat Msg Failed, err is " << err;
            return;
        }

        auto uid = jsonObj["uid"].toInt();
        qDebug() << "Receive offline Notify Success, uid is " << uid ;


        emit sig_notify_offline();

    });

    _handlers.insert(ID_HEARTBEAT_RSP,[this](ReqId id, int len, QByteArray data){
        Q_UNUSED(len);
        qDebug() << "handle id is " << id << " data is " << data;

        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);


        if (jsonDoc.isNull()) {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();

        if (!jsonObj.contains("error")) {
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "Heart Beat Msg Failed, err is Json Parse Err" << err;
            return;
        }

        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS) {
            qDebug() << "Heart Beat Msg Failed, err is " << err;
            return;
        }

        qDebug() << "Receive Heart Beat Msg Success" ;
        emit sig_heartbeat_ack(QDateTime::currentMSecsSinceEpoch());

    });

}

void TcpMgr::handleMsg(ReqId id, int len, QByteArray data)
{
   auto find_iter =  _handlers.find(id);
   if(find_iter == _handlers.end()){
        qDebug()<< "not found id ["<< id << "] to handle";
        return ;
   }

   find_iter.value()(id,len,data);
}

void TcpMgr::slot_tcp_connect(ServerInfo si)
{
    qDebug()<< "receive tcp connect signal";
    _host = si.Host.trimmed();
    if (_host == "0.0.0.0") {
        _host = "127.0.0.1";
    }

    bool portOk = false;
    const uint parsedPort = si.Port.trimmed().toUInt(&portOk);
    if (!portOk || parsedPort == 0 || parsedPort > std::numeric_limits<uint16_t>::max() || _host.isEmpty()) {
        qWarning() << "invalid chat server endpoint, host:" << _host << "port:" << si.Port;
        emit sig_con_success(false);
        return;
    }

    _port = static_cast<uint16_t>(parsedPort);
    if (_socket.state() != QAbstractSocket::UnconnectedState) {
        _socket.abort();
    }

    _buffer.clear();
    _b_recv_pending = false;
    _message_id = 0;
    _message_len = 0;
    _connecting = true;
    _connect_timeout_timer.start();
    qDebug() << "Connecting to chat server, host:" << _host << "port:" << _port;
    _socket.connectToHost(_host, _port);
}

void TcpMgr::slot_send_data(ReqId reqId, QByteArray dataBytes)
{
    uint16_t id = reqId;


    quint16 len = static_cast<quint16>(dataBytes.length());


    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);


    out.setByteOrder(QDataStream::BigEndian);


    out << id << len;


    block.append(dataBytes);


    _socket.write(block);
    qDebug() << "tcp mgr send byte data is " << block ;
}
