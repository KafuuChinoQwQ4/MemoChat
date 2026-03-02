#include "tcpmgr.h"
#include <limits>
#include <QAbstractSocket>
#include <QtEndian>
#include "usermgr.h"

TcpMgr::TcpMgr():_host(""),_port(0),_b_recv_pending(false),_message_id(0),_message_len(0)
{
    QObject::connect(&_socket, &QTcpSocket::connected, [&]() {
           qDebug() << "Connected to server!";
           _buffer.clear();
           _b_recv_pending = false;
           _message_id = 0;
           _message_len = 0;
           // 连接建立后发送消息
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

       //5.15 之后版本
//       QObject::connect(&_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred), [&](QAbstractSocket::SocketError socketError) {
//           Q_UNUSED(socketError)
//           qDebug() << "Error:" << _socket.errorString();
//       });

       // 处理错误（适用于Qt 5.15之前的版本）
        QObject::connect(&_socket, &QTcpSocket::errorOccurred,
                            [this](QAbstractSocket::SocketError socketError) {
               qDebug() << "Error:" << _socket.errorString() ;
               switch (socketError) {
                   case QTcpSocket::ConnectionRefusedError:
                       qDebug() << "Connection Refused!";
                       emit sig_con_success(false);
                       break;
                   case QTcpSocket::RemoteHostClosedError:
                       qDebug() << "Remote Host Closed Connection!";
                       break;
                   case QTcpSocket::HostNotFoundError:
                       qDebug() << "Host Not Found!";
                       emit sig_con_success(false);
                       break;
                   case QTcpSocket::SocketTimeoutError:
                       qDebug() << "Connection Timeout!";
                       emit sig_con_success(false);
                       break;
                   case QTcpSocket::NetworkError:
                       qDebug() << "Network Error!";
                       break;
                   default:
                       qDebug() << "Other Error!";
                       break;
               }
         });

        // 处理连接断开
        QObject::connect(&_socket, &QTcpSocket::disconnected, [&]() {
            qDebug() << "Disconnected from server.";
            _buffer.clear();
            _b_recv_pending = false;
            _message_id = 0;
            _message_len = 0;
            //并且发送通知到界面
            emit sig_connection_closed();
        });
        //连接发送信号用来发送数据
        QObject::connect(this, &TcpMgr::sig_send_data, this, &TcpMgr::slot_send_data);
        //注册消息
        initHandlers();
}

void TcpMgr::CloseConnection(){
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
        // 将QByteArray转换为QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        // 检查转换是否成功
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
        auto user_info = std::make_shared<UserInfo>(uid, name, nick, icon, sex,"",desc);
 
        UserMgr::GetInstance()->SetUserInfo(user_info);
        UserMgr::GetInstance()->SetToken(jsonObj["token"].toString());
        if(jsonObj.contains("apply_list")){
            UserMgr::GetInstance()->AppendApplyList(jsonObj["apply_list"].toArray());
        }

        //添加好友列表
        if (jsonObj.contains("friend_list")) {
            UserMgr::GetInstance()->AppendFriendList(jsonObj["friend_list"].toArray());
        }

        emit sig_swich_chatdlg();
    });


	_handlers.insert(ID_SEARCH_USER_RSP, [this](ReqId id, int len, QByteArray data) {
		Q_UNUSED(len);
		qDebug() << "handle id is " << id << " data is " << data;
		// 将QByteArray转换为QJsonDocument
		QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

		// 检查转换是否成功
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
               jsonObj["sex"].toInt(), jsonObj["icon"].toString());

        emit sig_user_search(search_info);
		});

	_handlers.insert(ID_NOTIFY_ADD_FRIEND_REQ, [this](ReqId id, int len, QByteArray data) {
		Q_UNUSED(len);
		qDebug() << "handle id is " << id << " data is " << data;
		// 将QByteArray转换为QJsonDocument
		QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

		// 检查转换是否成功
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

        auto apply_info = std::make_shared<AddFriendApply>(
                    from_uid, name, desc,
                      icon, nick, sex);

		emit sig_friend_apply(apply_info);
		});

    _handlers.insert(ID_NOTIFY_AUTH_FRIEND_REQ, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug() << "handle id is " << id << " data is " << data;
        // 将QByteArray转换为QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        // 检查转换是否成功
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

        auto auth_info = std::make_shared<AuthInfo>(from_uid,name,
                                                    nick, icon, sex);

        emit sig_add_auth_friend(auth_info);
        });

    _handlers.insert(ID_ADD_FRIEND_RSP, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug() << "handle id is " << id << " data is " << data;
        // 将QByteArray转换为QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        // 检查转换是否成功
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
        // 将QByteArray转换为QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        // 检查转换是否成功
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
        auto rsp = std::make_shared<AuthRsp>(uid, name, nick, icon, sex);
        emit sig_auth_rsp(rsp);

        qDebug() << "Auth Friend Success " ;
      });


    _handlers.insert(ID_TEXT_CHAT_MSG_RSP, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug() << "handle id is " << id << " data is " << data;
        // 将QByteArray转换为QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        // 检查转换是否成功
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
        //ui设置送达等标记 todo...
      });

    _handlers.insert(ID_NOTIFY_TEXT_CHAT_MSG_REQ, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug() << "handle id is " << id << " data is " << data;
        // 将QByteArray转换为QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        // 检查转换是否成功
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

    _handlers.insert(ID_GROUP_HISTORY_RSP, [this, parse_group_rsp](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        QJsonObject jsonObj;
        if (!parse_group_rsp(id, data, jsonObj)) {
            return;
        }

        const qint64 groupId = jsonObj.value("groupid").toVariant().toLongLong();
        const QJsonArray messages = jsonObj.value("messages").toArray();
        for (int i = messages.size() - 1; i >= 0; --i) {
            const QJsonObject one = messages.at(i).toObject();
            auto msg = std::make_shared<TextChatData>(one.value("msgid").toString(),
                                                      one.value("content").toString(),
                                                      one.value("fromuid").toInt(),
                                                      0);
            UserMgr::GetInstance()->AppendGroupChatMsg(groupId, msg);
        }
        emit sig_group_list_updated();
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

    _handlers.insert(ID_NOTIFY_GROUP_CHAT_MSG_REQ, [this](ReqId id, int len, QByteArray data) {
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
        const QJsonObject msgObj = jsonObj.value("msg").toObject();
        const QString fromName = jsonObj.value("from_nick").toString(jsonObj.value("from_name").toString());
        auto groupMsg = std::make_shared<GroupChatMsg>(groupId, fromUid, msgObj, fromName);
        emit sig_group_chat_msg(groupMsg);
      });

    _handlers.insert(ID_NOTIFY_OFF_LINE_REQ,[this](ReqId id, int len, QByteArray data){
        Q_UNUSED(len);
        qDebug() << "handle id is " << id << " data is " << data;
        // 将QByteArray转换为QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        // 检查转换是否成功
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
        //断开连接
        //并且发送通知到界面
        emit sig_notify_offline();

    });

    _handlers.insert(ID_HEARTBEAT_RSP,[this](ReqId id, int len, QByteArray data){
        Q_UNUSED(len);
        qDebug() << "handle id is " << id << " data is " << data;
        // 将QByteArray转换为QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        // 检查转换是否成功
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
    // 尝试连接到服务器
    qDebug() << "Connecting to server...";
    _host = si.Host;
    _port = static_cast<uint16_t>(si.Port.toUInt());
    _socket.connectToHost(si.Host, _port);
}

void TcpMgr::slot_send_data(ReqId reqId, QByteArray dataBytes)
{
    uint16_t id = reqId;

    // 计算长度（使用网络字节序转换）
    quint16 len = static_cast<quint16>(dataBytes.length());

    // 创建一个QByteArray用于存储要发送的所有数据
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);

    // 设置数据流使用网络字节序
    out.setByteOrder(QDataStream::BigEndian);

    // 写入ID和长度
    out << id << len;

    // 添加字符串数据
    block.append(dataBytes);

    // 发送数据
    _socket.write(block);
    qDebug() << "tcp mgr send byte data is " << block ;
}
