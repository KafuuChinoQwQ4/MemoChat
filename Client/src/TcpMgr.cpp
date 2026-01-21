#include "TcpMgr.h"
#include <QDebug>
#include <QDataStream>

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

    // 连接数据就绪 (处理粘包的关键)
    connect(&_socket, &QTcpSocket::readyRead, this, [this](){
        _buffer.append(_socket.readAll());

        QDataStream stream(&_buffer, QIODevice::ReadOnly);
        stream.setVersion(QDataStream::Qt_6_0); // 确保版本一致

        forever {
            // 1. 解析头部 (ID + Length) = 4字节
            if(!_b_recv_pending){
                if(_buffer.size() < sizeof(quint16) * 2){
                    return; // 头部都没收齐，继续等
                }
                
                // 读取头部
                stream >> _message_id >> _message_len;
                
                // 此时buffer指针已经移动了4个字节，我们需要把buffer前面的部分截断
                _buffer = _buffer.mid(sizeof(quint16) * 2);
                
                qDebug() << "Recv Msg Id:" << _message_id << " Len:" << _message_len;
            }

            // 2. 解析包体
            // 注意：这里 _buffer.size() 是剩余数据的长度
            if(_buffer.size() < _message_len){
                _b_recv_pending = true; // 说明包体还没收齐，标记一下，下次来了数据继续接在这个头后面
                return;
            }

            _b_recv_pending = false; // 包体收齐了
            
            // 读取包体数据
            QByteArray body = _buffer.mid(0, _message_len);
            QString data = QString::fromUtf8(body);
            
            qDebug() << "Recv Data:" << data;

            // TODO: 这里将来要分发数据到具体的业务逻辑 (emit sig_data_recv...)
            
            // 移除已处理的数据，准备处理下一个包
            _buffer = _buffer.mid(_message_len);
        }
    });

    // 连接错误处理
    connect(&_socket, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError socketError){
        qDebug() << "Tcp socket error:" << socketError;
        // 如果是正在连接中发生错误
        if(_socket.state() == QAbstractSocket::ConnectingState){
             emit sig_con_success(false);
        }
    });
}

void TcpMgr::slot_tcp_connect(ServerInfo si)
{
    qDebug() << "TcpMgr connecting to " << si.Host << ":" << si.Port;
    _host = si.Host;
    _port = si.Port.toUShort();
    
    // 如果之前连着，先断开
    if(_socket.state() != QAbstractSocket::UnconnectedState){
        _socket.abort();
    }
    
    _socket.connectToHost(_host, _port);
}

void TcpMgr::slot_send_data(ReqId reqId, QString data)
{
    uint16_t id = reqId;
    
    // 将字符串转为 UTF-8 字节
    QByteArray dataBytes = data.toUtf8();
    uint16_t len = dataBytes.size();

    // 构造数据包: [ID(2)][Len(2)][Data...]
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_0);
    
    // Qt 的 QDataStream 默认是大端序 (BigEndian)，符合网络标准
    out << id << len;
    block.append(dataBytes);

    _socket.write(block);
    qDebug() << "TcpMgr Sent Id:" << id << " Len:" << len << " Data:" << data;
}