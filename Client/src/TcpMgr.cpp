#include "TcpMgr.h"
#include "UserMgr.h" // 引用 UserMgr
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
    // 1. 注册处理函数
    // 处理登录回包 (ID_CHAT_LOGIN_RSP)
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

        // 登录成功，保存用户信息
        UserMgr::GetInstance()->SetUid(jsonObj["uid"].toInt());
        UserMgr::GetInstance()->SetName(jsonObj["name"].toString());
        UserMgr::GetInstance()->SetToken(jsonObj["token"].toString());
        
        // [关键] 发出切换界面信号！
        emit sig_swich_chatdlg();
    });

    // 2. 连接 socket 信号
    connect(&_socket, &QTcpSocket::connected, this, [this](){
        qDebug() << "TcpMgr::connected to server";
        emit sig_con_success(true);
    });

    connect(&_socket, &QTcpSocket::readyRead, this, [this](){
        _buffer.append(_socket.readAll());
        QDataStream stream(&_buffer, QIODevice::ReadOnly);
        stream.setVersion(QDataStream::Qt_6_0);

        forever {
            if(!_b_recv_pending){
                if(_buffer.size() < sizeof(quint16) * 2){
                    return; 
                }
                stream >> _message_id >> _message_len;
                _buffer = _buffer.mid(sizeof(quint16) * 2);
            }

            if(_buffer.size() < _message_len){
                _b_recv_pending = true;
                return;
            }

            _b_recv_pending = false;
            QByteArray body = _buffer.mid(0, _message_len);
            
            // [修改] 调用分发器处理消息，而不是直接打印
            handleMsg(ReqId(_message_id), _message_len, body);

            _buffer = _buffer.mid(_message_len);
        }
    });

    connect(&_socket, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError socketError){
        qDebug() << "Tcp socket error:" << socketError;
        if(_socket.state() == QAbstractSocket::ConnectingState){
             emit sig_con_success(false);
        }
    });
}

// [新增] 消息分发器实现
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