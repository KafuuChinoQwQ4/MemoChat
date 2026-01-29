#include "TcpMgr.h"
#include "UserMgr.h"
#include <QDebug>
#include <QDataStream>
#include <QJsonDocument>
#include <QJsonObject>

TcpMgr::TcpMgr() : _host(""), _port(0), _b_recv_pending(false), _message_id(0), _message_len(0)
{
    initHandlers();
}

TcpMgr::~TcpMgr() {
    _socket.close();
}

void TcpMgr::initHandlers() {
    // 连接建立成功
    connect(&_socket, &QTcpSocket::connected, this, [this](){
        qDebug() << "TcpMgr::connected to server";
        emit sig_con_success(true);
    });

    // 连接数据就绪
    connect(&_socket, &QTcpSocket::readyRead, this, [this](){
        _buffer.append(_socket.readAll());

        forever {
            // 【关键修改】stream 必须在循环内重新构造，以绑定最新的 _buffer
            QDataStream stream(&_buffer, QIODevice::ReadOnly);
            stream.setVersion(QDataStream::Qt_6_0);

            if(!_b_recv_pending){
                // 检查头部长度 (ID 2字节 + Len 2字节)
                if(_buffer.size() < sizeof(quint16) * 2){
                    return; // 数据不够，等下次
                }
                
                // 读取头部
                stream >> _message_id >> _message_len;
                
                // 移除头部 4 字节
                _buffer = _buffer.mid(sizeof(quint16) * 2);
                
                qDebug() << "Recv Msg Id:" << _message_id << " Len:" << _message_len;
            }

            // 检查包体长度
            if(_buffer.size() < _message_len){
                _b_recv_pending = true; // 包体不够，标记等待
                return;
            }

            _b_recv_pending = false; // 包体收齐
            
            // 读取包体
            QByteArray body = _buffer.mid(0, _message_len);
            
            // 分发处理
            handleMsg(ReqId(_message_id), _message_len, body);

            // 移除已处理的包体，准备处理下一个包
            _buffer = _buffer.mid(_message_len);
        }
    });

    // 错误处理
    connect(&_socket, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError socketError){
        qDebug() << "Tcp socket error:" << socketError;
        if(_socket.state() == QAbstractSocket::ConnectingState){
             emit sig_con_success(false);
        }
    });
    
    // 注册回调：登录回包
    _handlers.insert(ID_CHAT_LOGIN_RSP, [this](ReqId id, int len, QByteArray data){
        qDebug() << "handle id is " << id << " data is " << data;
        
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if(jsonDoc.isNull()){
           qDebug() << "Failed to create QJsonDocument.";
           return;
        }

        QJsonObject jsonObj = jsonDoc.object();
        if(!jsonObj.contains("error")){
            int err = ErrorCodes::ERR_JSON;
            emit sig_login_failed(err);
            return;
        }

        int err = jsonObj["error"].toInt();
        if(err != ErrorCodes::SUCCESS){
            qDebug() << "Login Failed, err is " << err ;
            emit sig_login_failed(err);
            return;
        }

        UserMgr::GetInstance()->SetUid(jsonObj["uid"].toInt());
        UserMgr::GetInstance()->SetName(jsonObj["name"].toString());
        UserMgr::GetInstance()->SetToken(jsonObj["token"].toString());
        
        // 发送跳转信号
        emit sig_swich_chatdlg();
    });
}

void TcpMgr::handleMsg(ReqId id, int len, QByteArray data)
{
   if(_handlers.contains(id)){
       _handlers[id](id, len, data);
   } else {
       qDebug()<< "not found id ["<< id << "] to handle";
   }
}

void TcpMgr::slot_tcp_connect(ServerInfo si)
{
    qDebug() << "TcpMgr connecting to " << si.Host << ":" << si.Port;
    _host = si.Host;
    _port = si.Port.toUShort();
    if(_socket.state() != QAbstractSocket::UnconnectedState){
        _socket.abort();
    }
    _socket.connectToHost(_host, _port);
}

void TcpMgr::slot_send_data(ReqId reqId, QString data)
{
    uint16_t id = reqId;
    QByteArray dataBytes = data.toUtf8();
    uint16_t len = dataBytes.size();

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_0);
    out << id << len;
    block.append(dataBytes);

    _socket.write(block);
}