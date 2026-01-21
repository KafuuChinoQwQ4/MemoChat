#pragma once
#include <QTcpSocket>
#include "Singleton.h"
#include "global.h"
#include <memory>

class TcpMgr : public QObject, public Singleton<TcpMgr>,
               public std::enable_shared_from_this<TcpMgr>
{
    Q_OBJECT
public:
    ~TcpMgr();
private:
    friend class Singleton<TcpMgr>;
    TcpMgr();
    
    void initHandlers(); // 初始化 socket 信号连接

    QTcpSocket _socket;
    QString _host;
    uint16_t _port;
    
    // 处理粘包/半包的缓存
    QByteArray _buffer;
    bool _b_recv_pending;
    quint16 _message_id;
    quint16 _message_len;

public slots:
    void slot_tcp_connect(ServerInfo si); // 连接服务器
    void slot_send_data(ReqId reqId, QString data); // 发送数据

signals:
    void sig_con_success(bool bsuccess); // 连接结果信号
    void sig_send_data(ReqId reqId, QString data); // 发送数据完成信号(可选)
    void sig_login_failed(int err); // 登录聊天服失败(可选)
};