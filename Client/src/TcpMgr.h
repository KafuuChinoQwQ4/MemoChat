#pragma once
#include <QTcpSocket>
#include "Singleton.h"
#include "global.h"
#include "userdata.h"
#include <memory>
#include <functional>
#include <QMap>

// 定义回调函数类型
typedef std::function<void(ReqId id, int len, QByteArray data)> ToDeal;

class TcpMgr : public QObject, public Singleton<TcpMgr>,
               public std::enable_shared_from_this<TcpMgr>
{
    Q_OBJECT
public:
    ~TcpMgr();
private:
    friend class Singleton<TcpMgr>;
    TcpMgr();
    
    void initHandlers(); 
    void handleMsg(ReqId id, int len, QByteArray data); // [新增] 消息分发器

    QTcpSocket _socket;
    QString _host;
    uint16_t _port;
    
    QByteArray _buffer;
    bool _b_recv_pending;
    quint16 _message_id;
    quint16 _message_len;
    
    // [新增] 消息处理器 Map
    QMap<ReqId, ToDeal> _handlers;

public slots:
    void slot_tcp_connect(ServerInfo si); 
    void slot_send_data(ReqId reqId, QString data); 

signals:
    void sig_con_success(bool bsuccess); 
    void sig_send_data(ReqId reqId, QString data); 
    void sig_login_failed(int err); 
    void sig_swich_chatdlg(); // [关键] 切换到聊天窗口信号
    void sig_user_search(std::shared_ptr<SearchInfo>);
    void sig_auth_rsp(std::shared_ptr<AuthRsp>);
};